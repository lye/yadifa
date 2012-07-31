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
/** @defgroup zone Functions used to sanitize a zone
 *  @ingroup dnsdb
 *  @brief Functions used to sanitize a zone
 *
 *  Functions used to sanitize a zone
 *
 * @{
 */

#include <dnscore/dnscore.h>
#include <dnscore/logger.h>
#include <dnscore/format.h>

#include "dnsdb/zdb_sanitize.h"

#include "dnsdb/zdb_rr_label.h"
#include "dnsdb/zdb_record.h"

#include "dnsdb/zdb_utils.h"
#include "dnsdb/zdb_error.h"

#include "dnsdb/zdb_dnsname.h"

extern logger_handle* g_database_logger;
#define MODULE_MSG_HANDLE g_database_logger

#define TYPE_OFFSET(t) ((t)>>3)
#define TYPE_BIT(t) (1<<((t)&7))

#define HAS_TYPE(t) ((types[TYPE_OFFSET(t)] & TYPE_BIT(t)) != 0)
#define SET_TYPE(t) (types[TYPE_OFFSET(t)] |= TYPE_BIT(t))
#define UNSET_TYPE(t) (types[TYPE_OFFSET(t)] &= ~TYPE_BIT(t))

#define CNAME_TYPE 5
#define NS_TYPE 2
#define DS_TYPE 43

static void
zdb_sanitize_log(dnsname_stack *dnsnamev, ya_result err)
{
    if(err & SANITY_UNEXPECTEDSOA)
    {
        log_warn("sanity: %{dnsnamestack} failed: unexpected SOA", dnsnamev);
    }
    if(err & SANITY_TOOMANYSOA)
    {
        log_err("sanity: %{dnsnamestack} failed: too many SOA", dnsnamev);
    }
    if(err & SANITY_CNAMENOTALONE)
    {
        log_warn("sanity: %{dnsnamestack} failed: CNAME must be alone", dnsnamev);
    }
    if(err & SANITY_UNEXPECTEDCNAME)
    {
        log_warn("sanity: %{dnsnamestack} failed: unexpected CNAME", dnsnamev);
    }
    if(err & SANITY_EXPECTEDNS)
    {
        log_warn("sanity: %{dnsnamestack} failed: expected NS", dnsnamev);
    }
    if(err & SANITY_UNEXPECTEDDS)
    {
        log_warn("sanity: %{dnsnamestack} failed: unexpected DS", dnsnamev);
    }
    if(err & SANITY_MUSTDROPZONE)
    {
        log_err("sanity: %{dnsnamestack} critical error : the zone will be dropped", dnsnamev);
    }
    if(err & SANITY_TRASHATDELEGATION)
    {
        log_warn("sanity: %{dnsnamestack} failed: delegation has unexpected records", dnsnamev);
    }
    if(err & SANITY_TRASHUNDERDELEGATION)
    {
        log_warn("sanity: %{dnsnamestack} failed: non-glue records found under delegation", dnsnamev);
    }
}

static void zdb_sanitize_rr_set_useless_glue(zdb_zone *zone, zdb_rr_label *label, dnsname_stack *name, zdb_rr_label** parent)
{
    zdb_rr_label** delegation = parent;
                
    while(*delegation != NULL && (((*delegation)->flags & ZDB_RR_LABEL_DELEGATION) == 0))
    {
        delegation--;
    }

    if(*delegation != NULL)
    {
        u16 ip_type = TYPE_A;

        zdb_packed_ttlrdata* delegation_ns_record_list_head = zdb_record_find(&(*delegation)->resource_record_set, TYPE_NS);

        for(;;)
        {
            zdb_packed_ttlrdata* ip_record_list = zdb_record_find(&label->resource_record_set, ip_type);

            zdb_packed_ttlrdata* delegation_ns_record_list = delegation_ns_record_list_head;

            while(ip_record_list != NULL)
            {

                bool matched = FALSE;

                while(delegation_ns_record_list != NULL)
                {
                    const u8* nameserver_name = ZDB_PACKEDRECORD_PTR_RDATAPTR(delegation_ns_record_list);

                    if(dnsname_equals_dnsname_stack(nameserver_name, name))
                    {
                        matched = TRUE;
                        break;
                    }

                    delegation_ns_record_list = delegation_ns_record_list->next;
                }

                if(!matched)
                {
                    /*
                    * Useless A/AAAA record
                    */

                    rdata_desc rdata;
                    rdata.type = ip_type;
                    rdata.len = ZDB_PACKEDRECORD_PTR_RDATASIZE(ip_record_list);
                    rdata.rdata = ZDB_PACKEDRECORD_PTR_RDATAPTR(ip_record_list);

                    log_warn("sanity: %{dnsname}: consider removing wrong glue: %{dnsnamestack} %{typerdatadesc}", zone->origin, name, &rdata);
                }

                ip_record_list = ip_record_list->next;
            }

            if(ip_type == TYPE_AAAA)
            {
                break;
            }

            ip_type = TYPE_AAAA;
        }
    }
}

static ya_result
zdb_sanitize_rr_set_ext(zdb_zone *zone, zdb_rr_label *label, dnsname_stack *name, u16 flags, zdb_rr_label** parent)
{
    /*
     * CNAME : nothing else than RRSIG & NSEC
     */

    u8 types[32]; /* I only really care about the types < 256, for any other one I'll use a boolean 'others' */

    /* record counts for ... (can overlap) */
    u32 not_cname_rrsig_nsec = 0;
    u32 not_ns_ds_nsec_rrsig = 0;
    u32 not_a_aaaa = 0;
    u32 a_aaaa = 0;
    u32 others = 0; /* outside the first 256 types */
    u32 soa = 0;
    u32 nsec = 0;
    bool ns_points_to_itself = FALSE;

    ya_result return_value = 0;
    
    bool isapex = zone->apex == label;

    ZEROMEMORY(types, sizeof(types));

    btree_iterator iter;
    btree_iterator_init(label->resource_record_set, &iter);

    while(btree_iterator_hasnext(&iter))
    {
        btree_node* node = btree_iterator_next_node(&iter);
        u16 type = node->hash;

        zdb_packed_ttlrdata* record_list = (zdb_packed_ttlrdata*)node->data;

        u32 ttl = record_list->ttl;
        while((record_list = record_list->next) != NULL)
        {
            record_list->ttl = ttl;
        }

        if((type & NU16(0xff00)) == 0)
        {
            if(type == TYPE_SOA)
            {
                soa++;
                not_cname_rrsig_nsec++;
                not_ns_ds_nsec_rrsig++;
                not_a_aaaa++;
            }
            else
            {
                switch(type)
                {
                    case TYPE_CNAME:
                        not_ns_ds_nsec_rrsig++;
                        not_a_aaaa++;
                        break;
                    case TYPE_NSEC:
                        nsec++;
                    case TYPE_RRSIG:
                        not_a_aaaa++;
                        break;
                    case TYPE_NS:
                    case TYPE_DS:                    
                        not_cname_rrsig_nsec++;
                        not_a_aaaa++;
                        break;
                    case TYPE_A:
                    case TYPE_AAAA:
                        not_cname_rrsig_nsec++;
                        not_ns_ds_nsec_rrsig++;
                        a_aaaa++;
                        break;
                    default:
                        not_cname_rrsig_nsec++;
                        not_ns_ds_nsec_rrsig++;
                        not_a_aaaa++;
                        break;
                }

                type = ntohs(type);

                SET_TYPE(type);
            }
        }
        else
        {
            others++;
        }
    }
    /*
    if(nsec > 0)
    {
        u32 max  = ((zone->apex->flags & ZDB_RR_LABEL_NSEC) == 0)?0:1;
        
        if(nsec > max)
        {
            return_value |= SANITY_TOOMANYNSEC | SANITY_MUSTDROPZONE;
        }
    }
    */
    /*
     * Too many SOAs or SOAs not at the APEX
     */

    if(soa > 0)
    {
        if(!isapex)
        {
            return_value |= SANITY_UNEXPECTEDSOA | SANITY_TOOMANYSOA;

            /*
             * Remove them all
             */

            zdb_record_delete(&label->resource_record_set, TYPE_SOA);
        }
        else
        {
            if(soa > 1)
            {
                return_value |= SANITY_TOOMANYSOA | SANITY_MUSTDROPZONE;

                /*
                 * DROP
                 */
            }
        }
    }
    
    if(HAS_TYPE(CNAME_TYPE))
    {
        /*
         * Cannot accept anything else than RRSIG & NSEC
         */

        if(!isapex)
        {
            if(others + not_cname_rrsig_nsec > 0)
            {
                return_value |= SANITY_CNAMENOTALONE;

                /*
                 * What do I remove ?
                 */
            }
        }
        else
        {
            /*
             * No CNAME at apex
             */

            return_value |= SANITY_UNEXPECTEDCNAME;
            
            /*
             * Remove them all
             */

            zdb_record_delete(&label->resource_record_set, TYPE_CNAME);
        }

        /*
         * Other DNS record types, such as NS, MX, PTR, SRV, etc. that point to other names should never point to a CNAME alias.
         * => insanely expensive to test
         */
    }

    if(HAS_TYPE(DS_TYPE))
    {
        if(!isapex)
        {
            /*
             * MUST have an NS with a DS
             */
            
            if(!HAS_TYPE(NS_TYPE))
            {
                return_value |= SANITY_EXPECTEDNS;

                zdb_record_delete(&label->resource_record_set, TYPE_DS);
            }
        }
        else
        {
            /*
             * cannot have a DS at apex
             */
            
            return_value |= SANITY_UNEXPECTEDDS;
            
            /*
             * Remove them all
             */

            zdb_record_delete(&label->resource_record_set, TYPE_DS);
        }
    }

    /*
     */

    if(HAS_TYPE(NS_TYPE))
    {
        /*
         * Need glue ?
         *
         * Has glue ?
         */
    }

    if(isapex)
    {
        /*
         * supposed to have one NS at apex
         */
        
        if(!HAS_TYPE(NS_TYPE))
        {
            return_value |= SANITY_EXPECTEDNS;

            /*
             * Just report it
             */
        }
    }
    else
    {
        if(flags & ZDB_RR_LABEL_DELEGATION)
        {
            /**
             * @todo The 3 #if 0 blocs are for the detection of NS that should have a glue but do not have one.
             */
            if(HAS_TYPE(NS_TYPE))
            {
                
            }
            else
            {
                return_value |= SANITY_EXPECTEDNS;
            }
            
            if(a_aaaa > 0 && parent != NULL)
            {
                zdb_sanitize_rr_set_useless_glue(zone, label, name, parent);
            }
            
            /*
             *  If we have anything excepct NS DS NSEC RRSIG ...
             */
            if(others + not_ns_ds_nsec_rrsig > 0)
            {
                /*
                 * If A/AAAA is all we have
                 */
                if(a_aaaa == others + not_ns_ds_nsec_rrsig)
                {
                    //zdb_packed_ttlrdata* record_list = ns_record_list;
                    zdb_packed_ttlrdata* record_list =zdb_record_find(&label->resource_record_set, TYPE_NS);

                    while(record_list != NULL)
                    {
                        if(dnsname_equals_dnsname_stack(ZDB_PACKEDRECORD_PTR_RDATAPTR(record_list), name))
                        {
                            ns_points_to_itself = TRUE;
                            break;
                        }

                        record_list = record_list->next;
                    }
                }

                if(! ns_points_to_itself)
                {
                    return_value |= SANITY_TRASHATDELEGATION;
                }
            }
        }
        else if(flags & ZDB_RR_LABEL_UNDERDELEGATION)
        {    
            if(a_aaaa > 0 && parent != NULL)
            {
                zdb_sanitize_rr_set_useless_glue(zone, label, name, parent);
            }
            if(others + not_a_aaaa > 0)
            {
                return_value |= SANITY_TRASHUNDERDELEGATION;
            }
        }
    }

    if(return_value != 0)
    {
        return_value |= SANITY_ERROR_BASE;
    }

    return return_value;
}

ya_result
zdb_sanitize_rr_set(zdb_zone *zone, zdb_rr_label *label)
{
    return zdb_sanitize_rr_set_ext(zone, label, NULL, 0, NULL);
}

static ya_result
zdb_sanitize_rr_label_ext(zdb_zone *zone, zdb_rr_label *label, dnsname_stack *name, u16 flags, zdb_rr_label** parent)
{
    /**
     *
     * For all labels: check the label is right.
     *
     */

    ya_result return_value;

    if((flags & (ZDB_RR_LABEL_DELEGATION|ZDB_RR_LABEL_UNDERDELEGATION)) != 0)
    {
        label->flags |= ZDB_RR_LABEL_UNDERDELEGATION;
    }
    else
    {
        label->flags &= ~ZDB_RR_LABEL_UNDERDELEGATION;
    }
    
    if(parent != NULL)
    {
        parent++;
        *parent = label;
    }
       
    if(FAIL(return_value = zdb_sanitize_rr_set_ext(zone, label, name, label->flags, parent)))
    {
        zdb_sanitize_log(name, return_value);

        if((return_value & SANITY_MUSTDROPZONE) != 0)
        {
            /**
             * Can stop here
             */

            return ERROR;
        }
    }
    
    u16 shutdown_test_countdown = 1000;
        
    dictionary_iterator iter;
    dictionary_iterator_init(&label->sub, &iter);
    while(dictionary_iterator_hasnext(&iter))
    {
        zdb_rr_label **sub_labelp = (zdb_rr_label**)dictionary_iterator_next(&iter);

        dnsname_stack_push_label(name, (*sub_labelp)->name);

        return_value = zdb_sanitize_rr_label_ext(zone, *sub_labelp, name, label->flags, parent);
        
        /*
         * If this label is under (or at) delegation
         *   For each A/AAAA record
         *     Ensure there are NS at delegation linked to said records.
         */

        dnsname_stack_pop_label(name);

        if(FAIL(return_value))
        {
            return return_value;
        }
        
        if(--shutdown_test_countdown == 0)
        {
            if(dnscore_shuttingdown())
            {
                return STOPPED_BY_APPLICATION_SHUTDOWN;
            }
            
            shutdown_test_countdown = 1000;
        }
    }

    return SUCCESS;
}

ya_result
zdb_sanitize_rr_label(zdb_zone *zone, zdb_rr_label *label, dnsname_stack *name)
{
    return zdb_sanitize_rr_label_ext(zone, label, name, 0, NULL);
}

ya_result
zdb_sanitize_rr_label_with_parent(zdb_zone *zone, zdb_rr_label *label, dnsname_stack *name)
{
    /*
     * the parent is at name.size-1
     * the zone starts at zone->origin_vector.size + 1
     * be sure not to do it at the apex
     */
    
    if(!ZDB_LABEL_ISAPEX(label))
    {
        int index = zone->origin_vector.size + 1;
        zdb_rr_label* label_stack[128];
        
#ifndef NDEBUG
        memset(label_stack, 0xff, sizeof(label_stack));
#endif
        
        zdb_rr_label *parent_label = zdb_rr_label_stack_find(zone->apex, name->labels, name->size-1, zone->origin_vector.size + 1);
        
        label_stack[0] = NULL;
        label_stack[1] = parent_label;
        
        return zdb_sanitize_rr_label_ext(zone, label, name, parent_label->flags, &label_stack[1]);
    }
    else
    {
        return zdb_sanitize_rr_label_ext(zone, label, name, 0, NULL);
    }    
}

ya_result
zdb_sanitize_zone(zdb_zone *zone)
{
    if(zone->apex == NULL)
    {
        return ERROR;
    }
    
    zdb_rr_label* label_stack[256];
    label_stack[0] = NULL;
    

    dnsname_stack name;
    dnsname_to_dnsname_stack(zone->origin, &name);

    return zdb_sanitize_rr_label_ext(zone, zone->apex, &name, 0, label_stack);
}

/** @} */
