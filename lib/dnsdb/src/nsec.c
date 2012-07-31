/*------------------------------------------------------------------------------
*
* Copyright (c) 2011, EURid. All rights reserved.
* The YADIFA TM software product is provided under the BSD 3-clause license:
* 
* Redistribution and use in source and binary forms, with or without 
* modification, are permitted provided that the following conditions
* are met:
*
*        * Redistributions of source code must retain the above copyright 
*          notice, this list of conditions and the following disclaimer.
*        * Redistributions in binary form must reproduce the above copyright 
*          notice, this list of conditions and the following disclaimer in the 
*          documentation and/or other materials provided with the distribution.
*        * Neither the name of EURid nor the names of its contributors may be 
*          used to endorse or promote products derived from this software 
*          without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*------------------------------------------------------------------------------
*
* DOCUMENTATION */
/** @defgroup nsec NSEC functions
 *  @ingroup dnsdbdnssec
 *  @brief
 *
 *
 *
 * @{
 */
/*------------------------------------------------------------------------------
 *
 * USE INCLUDES */
#include <stdio.h>
#include <stdlib.h>

#include <dnscore/dnscore.h>
#include <dnscore/dnsname.h>
#include <dnscore/logger.h>
#include <dnscore/format.h>

#include "dnsdb/treeset.h"

#include "dnsdb/zdb_rr_label.h"
#include "dnsdb/zdb_zone_label_iterator.h"
#include "dnsdb/zdb_dnsname.h"
#include "dnsdb/zdb_record.h"
#include "dnsdb/zdb_listener.h"

#include "dnsdb/rrsig.h"

#include "dnsdb/nsec.h"
#include "dnsdb/nsec_common.h"

/*
   Note : (rfc 4034)

   Because every authoritative name in a zone must be part of the NSEC
   chain, NSEC RRs must be present for names containing a CNAME RR.
   This is a change to the traditional DNS specification [RFC1034],
   which stated that if a CNAME is present for a name, it is the only
   type allowed at that name.  An RRSIG (see Section 3) and NSEC MUST
   exist for the same name as does a CNAME resource record in a signed
   zone.

   ...

   A sender MUST NOT use DNS name compression on the Next Domain Name
   field when transmitting an NSEC RR.

   ...

   Owner names of RRsets for which the given zone is not authoritative
   (such as glue records) MUST NOT be listed in the Next Domain Name
   unless at least one authoritative RRset exists at the same owner
   name.


 */

/*
 * The NSEC processing must be done AFTER the RRSIG one
 *
 * Assuming there are no NSEC:
 *
 * _ Explore the zone
 * _ Canonize names
 * _ Build NSEC records
 * _ NSEC records have to be found quicky
 * _ The NSEC records are either in an array, either double-linked-listed
 *
 * Label => find the nsec record
 * Label => NSEC HASH => find the nsec record
 *
 * If there are NSEC records ...
 *
 * (dyn-)adding a record means adding/changing a/the NSEC record
 * (dyn-)removing a record means removing/changing a/the NSEC record
 *
 * What's the most efficient way to do all this ?
 *
 * First issue : canonization.
 * ---------------------------
 *
 * The ordering of the name is by label depth.
 * So the best way I can think of is to sort the actual labels in the database.
 * But this is not possible.  Records are ordered by an hash.  This is one of
 * most important parts of the architecture.
 *
 * Still I have to have order, so it means that for each label I have to have
 * a sorted (canization-wise) array for the sub-labels.
 *
 * This means a new pointer for each label in a NSEC(3) zone (ARGH)
 * (zdb_rr_label)
 *
 * For the eu-zone and its 3M names, we are speaking of an overhead of 24MB
 * (64 bits)
 *
 * Ok, I can still live with that ...
 *
 * Could I also make it so that this pointer only exists in nsec-zones ?
 * No.  Because it means that an operation on the zone would basically require
 * a dup and complex size checks.  I don't think it's reasonable.
 *
 *
 * The NSEC record is stored with other records.
 * The NSEC3 is not stored with other records.
 *
 */

/* NSEC:
 *
 * At zone apex ...
 *
 * For each label
 *     If there are sub-labels
 *	     Get the sub-labels.
 *	     Canonize them.
 *	     Chain them, keep the chain.
 *	     Create the NSEC record.
 *	     Insert/Update the NSEC record in the label.
 *	     Sign the NSEC record.
 *
 *           The chain contains a link to the NSEC and the RRSIG
 *
 *           Recurse on sub-sub-label
 *
 */

#define MODULE_MSG_HANDLE g_dnssec_logger
extern logger_handle *g_dnssec_logger;

/*
 * New version of the NSEC handling
 *
 * Take all records
 * Prepare an NSEC record for them (using AVL)
 * For each entry in the AVL
 *  if the entry matches keep its signature and go to the next entry
 *  if the entry does not matches remove the old one and its signature and schedule for a new signature
 *
 */

static int nsec_update_zone_count = 0;

ya_result
nsec_update_zone(zdb_zone *zone)
{
    nsec_node *nsec_tree = NULL;
    soa_rdata soa;

    u8 name[MAX_DOMAIN_LENGTH];
    u8 inverse_name[MAX_DOMAIN_LENGTH];
    u8 tmp_bitmap[256 * (1 + 1 + 32)]; /* 'max window count' * 'max window length' */

    ya_result return_code;

    if(FAIL(return_code = zdb_zone_getsoa(zone, &soa)))
    {
        return return_code;
    }

    zdb_zone_label_iterator label_iterator;
    zdb_zone_label_iterator_init(zone, &label_iterator);

    while(zdb_zone_label_iterator_hasnext(&label_iterator))
    {
        u32 name_len = zdb_zone_label_iterator_nextname(&label_iterator, name);
        zdb_rr_label* label = zdb_zone_label_iterator_next(&label_iterator);

        if(zdb_rr_label_is_glue(label) || (label->resource_record_set == NULL))
        {
            continue;
        }

        nsec_inverse_name(inverse_name, name);

        nsec_node *node = nsec_avl_insert(&nsec_tree, inverse_name);
        node->label = label;
        label->nsec.nsec.node = node;
    }

    /*
     * Now that we have the NSEC chain
     */

    nsec_node *first_node;
    nsec_node *node;

    type_bit_maps_context tbmctx;

    nsec_avl_iterator nsec_iter;
    nsec_avl_iterator_init(&nsec_tree, &nsec_iter);

    if(nsec_avl_iterator_hasnext(&nsec_iter))
    {
        first_node = nsec_avl_iterator_next_node(&nsec_iter);

        node = first_node;

        do
        {
            nsec_node *next_node;

            nsec_update_zone_count++;

            if(nsec_avl_iterator_hasnext(&nsec_iter))
            {
                next_node = nsec_avl_iterator_next_node(&nsec_iter);
            }
            else
            {
                next_node = first_node;
            }

            /*
             * Compute the type bitmap
             */

            zdb_rr_label *label = node->label;

            u32 tbm_size = type_bit_maps_initialize(&tbmctx, label, TRUE, TRUE);
            type_bit_maps_write(tmp_bitmap, &tbmctx);

            nsec_inverse_name(name, next_node->inverse_relative_name);

            /*
             * Get the NSEC record
             */

            zdb_packed_ttlrdata *nsec_record;

            if((nsec_record = zdb_record_find(&label->resource_record_set, TYPE_NSEC)) != NULL)
            {
                /*
                 * has record -> compare the type and the nsec next
                 * if something does not match remove the record and its signature (no record)
                 *
                 */

                if(nsec_record->next == NULL)
                {
                    u8* rdata = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_record);
                    u16 size = ZDB_PACKEDRECORD_PTR_RDATASIZE(nsec_record);

                    u16 dname_len = dnsname_len(rdata);

                    if(dname_len < size)
                    {
                        u8* type_bitmap = &rdata[dname_len];

                        /*
                         * check the type bitmap
                         */

                        if(memcmp(tmp_bitmap, type_bitmap, size - dname_len) == 0)
                        {
                            /*
                             * check the nsec next
                             */

                            if(dnsname_equals(rdata, name))
                            {
                                /* All good */
                                
                                label->flags |= ZDB_RR_LABEL_NSEC;

                                node = next_node;
                                continue;
                            }
                        }
                    }
                }
                
                if(zdb_listener_notify_enabled())
                {
                    zdb_packed_ttlrdata *nsec_rec = nsec_record;

                    do
                    {
                        zdb_ttlrdata unpacked_ttlrdata;

                        unpacked_ttlrdata.ttl = nsec_rec->ttl;
                        unpacked_ttlrdata.rdata_size = ZDB_PACKEDRECORD_PTR_RDATASIZE(nsec_rec);
                        unpacked_ttlrdata.rdata_pointer = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_rec);

                        zdb_listener_notify_remove_record(name, TYPE_NSEC, &unpacked_ttlrdata);

                        nsec_rec = nsec_rec->next;
                    }
                    while(nsec_rec != NULL);
                }

                zdb_record_delete(&label->resource_record_set, TYPE_NSEC);

                rrsig_delete(name, label, TYPE_NSEC);

                nsec_record = NULL;
            }

            /*
             * no record -> create one and schedule a signature
             */

            if(nsec_record == NULL)
            {
                zdb_packed_ttlrdata *nsec_record;

                u16 dname_len = nsec_inverse_name(name, next_node->inverse_relative_name);
                u16 rdata_size = dname_len + tbm_size;

                ZDB_RECORD_ZALLOC_EMPTY(nsec_record, soa.minimum, rdata_size);

                u8* rdata = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_record);

                memcpy(rdata, name, dname_len);
                rdata += dname_len;

                memcpy(rdata, tmp_bitmap, tbm_size);
                
                zdb_record_insert(&label->resource_record_set, TYPE_NSEC, nsec_record);

                /*
                 * Schedule a signature
                 */
            }

            label->flags |= ZDB_RR_LABEL_NSEC;

            node = next_node;
        }
        while(node != first_node);
    }

    zone->nsec.nsec = nsec_tree;

    return SUCCESS;
}

/**
 * Reverses the labels of the fqdn
 *
 * @param inverse_name
 * @param name
 * @return
 *
 * 3 www 5 eurid 2 eu 0
 *
 * 3 5 2 0
 */

u32
nsec_inverse_name(u8 *inverse_name, const u8 *name)
{
    dnslabel_vector labels;

    s32 vtop = dnsname_to_dnslabel_vector(name, labels);
    return dnslabel_stack_to_dnsname(labels, vtop, inverse_name);
}

/**
 * Verifies and, if needed, update the NSEC record.
 * There WILL be an NSEC record in the label at the end of the call.
 * It does NOT create the NSEC node (needs it created already).
 * It does NOT check for the relevancy of the NSEC record.
 *
 * @param label
 * @param node
 * @param next_node
 * @param name
 * @param ttl
 * @return
 */

bool
nsec_update_label_record(zdb_rr_label *label, nsec_node *node, nsec_node *next_node, u8 *name, u32 ttl)
{
    type_bit_maps_context tbmctx;
    u8 tmp_bitmap[256 * (1 + 1 + 32)]; /* 'max window count' * 'max window length' */
    u32 tbm_size = type_bit_maps_initialize(&tbmctx, label, TRUE, TRUE);

    type_bit_maps_write(tmp_bitmap, &tbmctx);

    /*
     * Get the NSEC record
     */

    zdb_packed_ttlrdata *nsec_record;

    if((nsec_record = zdb_record_find(&label->resource_record_set, TYPE_NSEC)) != NULL)
    {
        /*
         * has record -> compare the type and the nsec next
         * if something does not match remove the record and its signature (no record)
         *
         */

        log_debug("nsec_update_label_record: [%{dnsname}] %{dnsname} (=> %{dnsname}) updating record.", name, node->inverse_relative_name, next_node->inverse_relative_name);

        /*
         * If there is more than one record, clean-up
         */


        if(nsec_record->next == NULL)
        {
            u8* rdata = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_record);
            u16 size = ZDB_PACKEDRECORD_PTR_RDATASIZE(nsec_record);

            u16 dname_len = dnsname_len(rdata);

            if(dname_len < size)
            {
                u8* type_bitmap = &rdata[dname_len];

                /*
                 * check the type bitmap
                 */

                if(memcmp(tmp_bitmap, type_bitmap, size - dname_len) == 0)
                {
                    /*
                     * check the nsec next
                     */

                    if(dnsname_equals(rdata, name))
                    {
                        /* All good */

                        return FALSE;
                    }
                }
            }    
        }

        if(zdb_listener_notify_enabled())
        {
            zdb_packed_ttlrdata *nsec_rec = nsec_record;

            do
            {
                zdb_ttlrdata unpacked_ttlrdata;

                unpacked_ttlrdata.ttl = nsec_rec->ttl;
                unpacked_ttlrdata.rdata_size = ZDB_PACKEDRECORD_PTR_RDATASIZE(nsec_rec);
                unpacked_ttlrdata.rdata_pointer = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_rec);

                zdb_listener_notify_remove_record(name, TYPE_NSEC, &unpacked_ttlrdata);

                nsec_rec = nsec_rec->next;
            }
            while(nsec_rec != NULL);
        }

        zdb_record_delete(&label->resource_record_set, TYPE_NSEC);

        rrsig_delete(name, label, TYPE_NSEC);

        nsec_record = NULL;
    }

    /*
     * no record -> create one and schedule a signature
     */

    if(nsec_record == NULL)
    {
        zdb_packed_ttlrdata *nsec_record;
        u8 next_name[256];

        log_debug("nsec_update_label_record: [%{dnsname}] %{dnsname} (=> %{dnsname}) building new record.", name, node->inverse_relative_name, next_node->inverse_relative_name);

        u16 dname_len = nsec_inverse_name(next_name, next_node->inverse_relative_name);
        u16 rdata_size = dname_len + tbm_size;

        ZDB_RECORD_ZALLOC_EMPTY(nsec_record, ttl, rdata_size);

        u8* rdata = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_record);

        memcpy(rdata, next_name, dname_len);
        rdata += dname_len;

        memcpy(rdata, tmp_bitmap, tbm_size);

        zdb_record_insert(&label->resource_record_set, TYPE_NSEC, nsec_record);
        
        if(zdb_listener_notify_enabled())
        {        
            dnsname_vector name_path;

            zdb_ttlrdata unpacked_ttlrdata;
            
            unpacked_ttlrdata.ttl = ttl;
            unpacked_ttlrdata.rdata_size = rdata_size;
            unpacked_ttlrdata.rdata_pointer = ZDB_PACKEDRECORD_PTR_RDATAPTR(nsec_record);

            dnsname_to_dnsname_vector(name, &name_path);

            zdb_listener_notify_add_record(name_path.labels, name_path.size, TYPE_NSEC, &unpacked_ttlrdata);
        }

        /*
         * Schedule a signature
         */
    }

    label->flags |= ZDB_RR_LABEL_NSEC;

    return TRUE;
}

/**
 * Creates the NSEC node, link it to the label.
 *
 * @param zone
 * @param label
 * @param labels
 * @param labels_top
 * @return
 */

nsec_node *
nsec_update_label_node(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top)
{
    u8 inverse_name[MAX_DOMAIN_LENGTH];

    dnslabel_stack_to_dnsname(labels, labels_top, inverse_name);

    nsec_node *node = nsec_avl_insert(&zone->nsec.nsec, inverse_name);
    node->label = label;
    label->nsec.nsec.node = node;

    log_debug("nsec_update_label_node: %{dnsname}", node->inverse_relative_name);
    
    return node;
}

/**
 * 
 * Unlink the NSEC node from the label, then deletes said node from the chain.
 * 
 * @param zone
 * @param label
 * @param labels
 * @param labels_top
 * @return 
 */

bool
nsec_delete_label_node(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top)
{
    u8 inverse_name[MAX_DOMAIN_LENGTH];

    dnslabel_stack_to_dnsname(labels, labels_top, inverse_name);

    nsec_node *node = nsec_avl_find(&zone->nsec.nsec, inverse_name);
    
    if(node != NULL)
    {
        node->label->nsec.nsec.node = NULL;
        node->label = NULL;
        nsec_avl_delete(&zone->nsec.nsec, inverse_name);

        log_debug("nsec_delete_label_node: %{dnsname}", inverse_name);
        
        return TRUE;
    }
    else
    {
        log_debug("nsec_delete_label_node: %{dnsname} has not been found", inverse_name);
        
        return FALSE;
    }
}

/**
 * Creates the NSEC node, creates or update the NSEC record
 * 
 * @param zone
 * @param label
 * @param labels
 * @param labels_top
 */

void
nsec_update_label(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top)
{
    soa_rdata soa;
    u8 name[MAX_DOMAIN_LENGTH];

    zdb_zone_getsoa(zone, &soa);

    /* Create or get the node */

    nsec_node *node = nsec_update_label_node(zone, label, labels, labels_top);

    /* Get the next node */

    nsec_node *next_node = nsec_avl_node_mod_next(node);

    dnslabel_vector_to_dnsname(labels, labels_top, name);

    nsec_update_label_record(label, node, next_node, name, soa.minimum);
}

void
nsec_destroy_zone(zdb_zone *zone)
{
    nsec_avl_iterator iter;
    nsec_avl_iterator_init(&zone->nsec.nsec,&iter);

    while(nsec_avl_iterator_hasnext(&iter))
    {
        nsec_node *node = nsec_avl_iterator_next_node(&iter);
        node->label->nsec.nsec.node = NULL;
        node->label->flags &= ~ZDB_RR_LABEL_NSEC;
    }

    nsec_avl_destroy(&zone->nsec.nsec);    
}

/**/

static int
treeset_dnsname_compare(const void *a, const void *b)
{
    const u8 *dnsname_a = (const u8*)a;
    const u8 *dnsname_b = (const u8*)b;

    return dnsname_compare(dnsname_a,dnsname_b);
}

void
nsec_icmtl_replay_init(nsec_icmtl_replay *replay, zdb_zone *zone)
{
    ZEROMEMORY(replay, sizeof(nsec_icmtl_replay));

    replay->nsec_add.compare = treeset_dnsname_compare;
    replay->nsec_del.compare = treeset_dnsname_compare;
    replay->zone = zone;
}

static void
nsec3_icmtl_destroy_nsec(treeset_tree *tree)
{
    if(!treeset_avl_isempty(tree))
    {
        treeset_avl_iterator n3p_avl_iter;
        treeset_avl_iterator_init(tree, &n3p_avl_iter);

        while(treeset_avl_iterator_hasnext(&n3p_avl_iter))
        {
            treeset_node *node = treeset_avl_iterator_next_node(&n3p_avl_iter);
            free(node->key);
            node->key = NULL;
            node->data = NULL;
        }
        
        treeset_avl_destroy(tree);
    }
}

void
nsec_icmtl_replay_destroy(nsec_icmtl_replay *replay)
{
    nsec3_icmtl_destroy_nsec(&replay->nsec_add);
    nsec3_icmtl_destroy_nsec(&replay->nsec_del);
    
    replay->zone = NULL;
}

void
nsec_icmtl_replay_nsec_del(nsec_icmtl_replay *replay, const u8* fqdn)
{
    treeset_node *node = treeset_avl_insert(&replay->nsec_del, (u8*)fqdn);
    
    node->key = dnsname_dup(fqdn);
    node->data = NULL;
}

void
nsec_icmtl_replay_nsec_add(nsec_icmtl_replay *replay, const u8* fqdn)
{
    treeset_node *node = treeset_avl_insert(&replay->nsec_add, (u8*)fqdn);

    node->key = dnsname_dup(fqdn);
    node->data = NULL;
}

void
nsec_icmtl_replay_execute(nsec_icmtl_replay *replay)
{
    if(!treeset_avl_isempty(&replay->nsec_del))
    {
        /* stuff to delete */

        treeset_avl_iterator ts_avl_iter;
        treeset_avl_iterator_init(&replay->nsec_del, &ts_avl_iter);

        while(treeset_avl_iterator_hasnext(&ts_avl_iter))
        {
            treeset_node *node = treeset_avl_iterator_next_node(&ts_avl_iter);
            u8 *fqdn = (u8*)node->key;

            log_debug("icmtl replay: NSEC: post/del %{dnsname}", fqdn);

            treeset_node *add_node;

            if((add_node = treeset_avl_find(&replay->nsec_add, fqdn)) != NULL)
            {
                /*
                 *  del and add => nothing to do (almost)
                 *
                 *  NOTE: I have to ensure that the label link is right (if the label has ENTIERLY been destroyed,
                 *        then re-made, this will break)
                 */

                log_debug("icmtl replay: NSEC: upd %{dnsname}", fqdn);

                /*
                 *
                 */

                u8* add_key = add_node->key;
                treeset_avl_delete(&replay->nsec_add, fqdn);
                
                free(add_key);
            }
            else
            {
                log_debug("icmtl replay: NSEC: del %{dnsname}", fqdn);

                /*
                 * The node has to be deleted
                 */

                dnslabel_vector labels;
                s32 labels_top = dnsname_to_dnslabel_vector(fqdn, labels);
                
                zdb_rr_label* label = zdb_rr_label_find_exact(replay->zone->apex, labels, labels_top);

                nsec_delete_label_node(replay->zone, label, labels, labels_top);
            }

            free(fqdn);
        }

        treeset_avl_destroy(&replay->nsec_del);
    }
    if(!treeset_avl_isempty(&replay->nsec_add))
    {
        /* stuff to add */

        treeset_avl_iterator ts_avl_iter;
        treeset_avl_iterator_init(&replay->nsec_add, &ts_avl_iter);

        while(treeset_avl_iterator_hasnext(&ts_avl_iter))
        {
            treeset_node *node = treeset_avl_iterator_next_node(&ts_avl_iter);
            u8 *fqdn = (u8*)node->key;

            log_debug("icmtl replay: NSEC: add %{dnsname}", fqdn);

            /*
             * The node must be added.  It should not exist already.
             * After all changes (del/upd/add) all the added records should be matched again (check)
             */

            dnslabel_vector labels;
            s32 labels_top = dnsname_to_dnslabel_vector(fqdn, labels);

            zdb_rr_label* label = zdb_rr_label_find_exact(replay->zone->apex, labels, labels_top - replay->zone->origin_vector.size - 1);

            nsec_update_label_node(replay->zone, label, labels, labels_top);

            free(fqdn);
        }

        treeset_avl_destroy(&replay->nsec_add);
    }
}

/**
 *
 * Find the label that has got the right NSEC interval for "nextname"
 *
 * @param zone
 * @param name_vector
 * @param dname_out
 * @return
 */

zdb_rr_label *
nsec_find_interval(const zdb_zone *zone, const dnsname_vector *name_vector, u8 *dname_out)
{
    u8 dname_inverted[MAX_DOMAIN_LENGTH];
    
    dnslabel_stack_to_dnsname(name_vector->labels, name_vector->size, dname_inverted);
    
    nsec_node *node = nsec_avl_find_interval_start(&zone->nsec.nsec, dname_inverted);

    nsec_inverse_name(dname_out, node->inverse_relative_name);

    return node->label;
}
/*
void
nsec_find_interval_and_wild(zdb_zone *zone, const dnsname_vector *name_vector, zdb_rr_label **label, u8 *dname_out, zdb_rr_label **wild_label, u8 *wild_dname_out)
{
    u8 dname_inverted[MAX_DOMAIN_LENGTH + 2];
    
    s32 len = dnslabel_stack_to_dnsname(name_vector->labels, name_vector->size, dname_inverted);
    
    nsec_node *node = nsec_avl_find_interval_start(&zone->nsec.nsec, dname_inverted);
    
    nsec_inverse_name(dname_out, node->inverse_relative_name);
    
    dname_inverted[len-1] = (u8)1;
    dname_inverted[len+0] = (u8)'*';
    dname_inverted[len+1] = (u8)0;
    
    nsec_node *wild_node = nsec_avl_find_interval_start(&zone->nsec.nsec, dname_inverted);
    
    if(wild_node != node)
    {
        nsec_inverse_name(wild_dname_out, wild_node->inverse_relative_name);
    }
    
    *label = node->label;
    *wild_label = wild_node->label;
}
*/
void
nsec_name_error(const zdb_zone* zone, const dnsname_vector *name, s32 closest_index,
                 u8* out_encloser_nsec_name,
                 zdb_rr_label** out_encloser_nsec_label,
                 u8* out_wild_encloser_nsec_name,
                 zdb_rr_label** out_wildencloser_nsec_label
                 )
{
    u8 dname_inverted[MAX_DOMAIN_LENGTH + 2];
    
    s32 len = dnslabel_stack_to_dnsname(name->labels, name->size, dname_inverted);
    
    nsec_node *node = nsec_avl_find_interval_start(&zone->nsec.nsec, dname_inverted);
    
    nsec_inverse_name(out_encloser_nsec_name, node->inverse_relative_name);
    
    /*
    dname_inverted[len-1] = (u8)1;
    dname_inverted[len+0] = (u8)'*';
    dname_inverted[len+1] = (u8)0;
    */
    
    //if(name->size == closest_index)
    
    len = dnslabel_stack_to_dnsname(&name->labels[closest_index], name->size - closest_index, dname_inverted);
    
    nsec_node *wild_node = nsec_avl_find_interval_start(&zone->nsec.nsec, dname_inverted);
    
    if(wild_node != node)
    {
        nsec_inverse_name(out_wild_encloser_nsec_name, wild_node->inverse_relative_name);
    }
    
    *out_encloser_nsec_label = node->label;
    *out_wildencloser_nsec_label = wild_node->label;
}

void
nsec_logdump_tree(zdb_zone *zone)
{
    log_debug("dumping zone %{dnsname} nsec tree", zone->origin);

    nsec_avl_iterator iter;
    nsec_avl_iterator_init(&zone->nsec.nsec, &iter);
    while(nsec_avl_iterator_hasnext(&iter))
    {
        nsec_node *node = nsec_avl_iterator_next_node(&iter);

        log_debug("%{dnsname}", node->inverse_relative_name);
    }
    log_debug("done dumping zone %{dnsname} nsec tree", zone->origin);
}

/** @} */

/*----------------------------------------------------------------------------*/

