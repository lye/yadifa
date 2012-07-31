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
/** @defgroup nsec3 NSEC3 functions
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

#include <dnscore/output_stream.h>

#include "dnsdb/zdb_record.h"

#include "dnsdb/nsec3_item.h"
#include "dnsdb/nsec3_owner.h"
#include "dnsdb/nsec3_zone.h"

#include "dnsdb/rrsig.h"

#include <dnscore/base32hex.h>

nsec3_zone_item*
nsec3_zone_item_find(nsec3_zone* n3, const u8* digest)
{
    return nsec3_avl_find_interval_start(&n3->items, (u8*)digest);
}

nsec3_zone_item*
nsec3_zone_item_find_by_name(nsec3_zone* n3, const u8* nsec3_label)
{
    u8 digest[256];
    
    ya_result digest_len = base32hex_decode((char*)&nsec3_label[1], nsec3_label[0], &digest[1]);
    
    if(ISOK(digest_len))
    {
        digest[0] = digest_len;

        return nsec3_avl_find(&n3->items, digest);
    }
    else
    {
        return NULL;
    }
}

nsec3_zone_item*
nsec3_zone_item_find_by_name_ext(zdb_zone *zone, const u8 *fqdn, nsec3_zone **out_n3)
{
    nsec3_zone *n3 = zone->nsec.nsec3;
    nsec3_zone_item *n3zi = NULL;

    while(n3 != NULL)
    {
        if((n3zi = nsec3_zone_item_find_by_name(n3, fqdn)) != NULL)
        {
            break;
        }

        n3 = n3->next;
    }

    if(out_n3 != NULL)
    {
        *out_n3 = n3;
    }

    return n3zi;
}

nsec3_zone_item*
nsec3_zone_item_find_by_record(zdb_zone *zone, const u8 *fqdn, u16 rdata_size, const u8 *rdata)
{
    nsec3_zone *n3 = nsec3_zone_get_from_rdata(zone, rdata_size, rdata);
    
    nsec3_zone_item *n3zi = NULL;
    
    if(n3 != NULL)
    {
        n3zi = nsec3_zone_item_find_by_name(n3, fqdn);
    }

    return n3zi;
}

bool
nsec3_zone_item_equals_rdata(nsec3_zone* n3,
                             nsec3_zone_item *item,
                             u16 rdata_size,
                             const u8* rdata)
{
    u32 param_rdata_size = NSEC3_ZONE_RDATA_SIZE(n3);
    u8 hash_len = NSEC3_NODE_DIGEST_SIZE(item);
    u32 type_bit_maps_size = item->type_bit_maps_size;

    u32 item_rdata_size = param_rdata_size + 1 + hash_len + type_bit_maps_size;

    if(item_rdata_size != rdata_size)
    {
        return FALSE;
    }

    /* Do not check the flags */

    if(nsec3_zone_rdata_compare(rdata, n3->rdata) != 0)
    {
        return FALSE;
    }

    const u8 *p = &rdata[param_rdata_size];

    nsec3_zone_item* next = nsec3_avl_node_mod_next(item);

    if(memcmp(p, next->digest, hash_len + 1) != 0)
    {
#if DEBUG_LEVEL >= 9
        nsec3_avl_find_debug(&n3->items, item->digest);
        nsec3_avl_find_debug(&n3->items, next->digest);
        bool exists = nsec3_avl_find_debug(&n3->items, p) != NULL;

        OSLDEBUG(9, termout, "REJECT: %{digest32h} NSEC3 ... %{digest32h} was expected to be followed by %{digest32h} (%i)",
                 item->digest, next->digest, p, exists);
#endif
        return FALSE;
    }

    p += hash_len + 1;

    return memcmp(p, item->type_bit_maps, item->type_bit_maps_size) == 0;
}

/**
 *
 * @param n3
 * @param item
 * @param origin
 * @param out_owner
 * @param out_nsec3         return value, if not NULL, it is allocated by a malloc
 * @param out_nsec3_rrsig   return value, if not NULL, it's a reference into the DB
 */

void
nsec3_zone_item_to_zdb_packed_ttlrdata(
                                       nsec3_zone* n3,
                                       nsec3_zone_item* item,
                                       u8* origin,
                                       u8* out_owner, /* dnsname */
                                       u32 ttl,
                                       zdb_packed_ttlrdata** out_nsec3,
                                       zdb_packed_ttlrdata** out_nsec3_rrsig)
{
    u32 param_rdata_size = NSEC3_ZONE_RDATA_SIZE(n3);
    u8 hash_len = NSEC3_NODE_DIGEST_SIZE(item);
    u32 type_bit_maps_size = item->type_bit_maps_size;

    /* Whatever the editor says: rdata_size is used. */
    u32 rdata_size = param_rdata_size + 1 + hash_len + type_bit_maps_size;

    zdb_packed_ttlrdata* nsec3;

    /*
     * NOTE: ZALLOC SHOULD NEVER BE USED IN MT
     *
     */

    ZDB_RECORD_MALLOC_EMPTY(nsec3, ttl, rdata_size);

    nsec3->next = NULL;

    u8* p = &nsec3->rdata_start[0];

    MEMCOPY(p, &n3->rdata[0], param_rdata_size);
    p += param_rdata_size;

    nsec3_zone_item* next = nsec3_avl_node_mod_next(item);

    MEMCOPY(p, next->digest, hash_len + 1);
    p += hash_len + 1;

    MEMCOPY(p, item->type_bit_maps, item->type_bit_maps_size);

    u32 b32_len = base32hex_encode(NSEC3_NODE_DIGEST_PTR(item), hash_len, (char*)& out_owner[1]);
    out_owner[0] = b32_len;

    u32 origin_len = dnsname_len(origin);
    MEMCOPY(&out_owner[1 + b32_len], origin, origin_len);

    nsec3->rdata_start[1] = item->flags&1; /* Opt-Out or Opt-In */

    *out_nsec3 = nsec3;
    *out_nsec3_rrsig = item->rrsig;
}

u32
nsec3_zone_item_get_label(nsec3_zone_item* item,
                          u8* output_buffer,
                          u32 buffer_size
                          )
{
    zassert(buffer_size >= 128);

    u8 hash_len = NSEC3_NODE_DIGEST_SIZE(item);
    u32 b32_len = base32hex_encode(NSEC3_NODE_DIGEST_PTR(item), hash_len, (char*)&output_buffer[1]);
    output_buffer[0] = b32_len;

    return b32_len + 1;
}

void
nsec3_zone_item_write_owner(output_stream* os,
                            nsec3_zone_item* item,
                            u8* origin
                            )
{
    u8 tmp[128]; /* enough to get a 64 bit digest printed as base32hex */

    u32 label_len = nsec3_zone_item_get_label(item, tmp, sizeof (tmp));

    output_stream_write(os, tmp, label_len);

    u32 origin_len = dnsname_len(origin);

    output_stream_write(os, origin, origin_len);
}

void
nsec3_zone_item_to_output_stream(output_stream* os,
                                 nsec3_zone* n3,
                                 nsec3_zone_item* item,
                                 u8* origin,
                                 u32 ttl)
{
    u8 tmp[128]; /* enough to get a 64 bit digest printed as base32hex */

    u32 param_rdata_size = NSEC3_ZONE_RDATA_SIZE(n3);
    u8 hash_len = NSEC3_NODE_DIGEST_SIZE(item);
    u32 type_bit_maps_size = item->type_bit_maps_size;

    /* Whatever the editor says: rdata_size is used. */
    u32 rdata_size = param_rdata_size + 1 + hash_len + type_bit_maps_size;

    u32 b32_len = base32hex_encode(NSEC3_NODE_DIGEST_PTR(item), hash_len, (char*)&tmp[1]);
    tmp[0] = b32_len;

    /* NAME */

    output_stream_write(os, tmp, b32_len + 1);

    u32 origin_len = dnsname_len(origin);

    output_stream_write(os, origin, origin_len);

    /* TYPE */

    output_stream_write_u16(os, TYPE_NSEC3); /** @note NATIVETYPE */

    /* CLASS */

    output_stream_write_u16(os, CLASS_IN); /** @note NATIVECLASS */

    /* TTL */

    output_stream_write_nu32(os, ttl);

    /* RDATA SIZE */

    output_stream_write_nu16(os, rdata_size);

    /* RDATA */

    output_stream_write_u8(os, n3->rdata[0]);

    output_stream_write_u8(os, item->flags);

    output_stream_write(os, &n3->rdata[2], param_rdata_size - 2);

    nsec3_zone_item* next = nsec3_avl_node_mod_next(item);

    output_stream_write(os, next->digest, hash_len + 1);

    output_stream_write(os, item->type_bit_maps, item->type_bit_maps_size);
}

void
nsec3_zone_item_rrsig_del_by_keytag(nsec3_zone_item *item, u16 native_key_tag)
{
    if(item->rrsig != NULL)
    {
        zdb_packed_ttlrdata **rrsigp = &item->rrsig;

        do
        {
            zdb_packed_ttlrdata *rrsig = *rrsigp;
            
            if(RRSIG_KEY_NATIVETAG(rrsig) ==  native_key_tag)
            {
                /* Remove from the list */
                *rrsigp = rrsig->next;
                ZDB_RECORD_ZFREE(rrsig);
                break;
            }

            rrsigp = &rrsig->next;
        }
        while(*rrsigp != NULL);
    }
}

void
nsec3_zone_item_rrsig_del(nsec3_zone_item *item, zdb_ttlrdata *nsec3_rrsig)
{
    if(item->rrsig != NULL)
    {
        do
        {
            zdb_packed_ttlrdata **rrsigp = &item->rrsig;
            do
            {
                zdb_packed_ttlrdata *rrsig = *rrsigp;

                if(zdb_record_equals_unpacked(rrsig, nsec3_rrsig))
                {
                    /* Remove from the list */
                    *rrsigp = rrsig->next;
                    ZDB_RECORD_ZFREE(rrsig);
                    break;
                }

                rrsigp = &rrsig->next;
            }
            while(*rrsigp != NULL);
            
            nsec3_rrsig = nsec3_rrsig->next;
        }
        while(nsec3_rrsig != NULL);
    }
}

void
nsec3_zone_item_rrsig_add(nsec3_zone_item *item, zdb_packed_ttlrdata *nsec3_rrsig)
{    
    if(item->rrsig == NULL)
    {
        item->rrsig = nsec3_rrsig;
    }
    else
    {        
        zdb_packed_ttlrdata *good = NULL;
        zdb_packed_ttlrdata *rrsig;
        
        while(nsec3_rrsig != NULL)
        {
            /* look for the first item on the list */

            rrsig = item->rrsig;
            
            bool add = TRUE;
            
            do
            {
                /* Replaces another signature ? */

                if(zdb_record_equals(rrsig, nsec3_rrsig))
                {               
                    add = FALSE;
                    break;                    
                }
                
                rrsig = rrsig->next;
            }
            while(rrsig != NULL);
            
            zdb_packed_ttlrdata *tmp = nsec3_rrsig->next;
            
            if(add)
            {
                nsec3_rrsig->next = good;
                good = nsec3_rrsig;
            }
            else
            {
                ZDB_RECORD_ZFREE(nsec3_rrsig);
            }
            
            nsec3_rrsig = tmp;
        }
        
        if(good != NULL)
        {
            rrsig = item->rrsig;
            
            while(rrsig->next != NULL)
            {
                rrsig = rrsig->next;
            }
            
            rrsig->next = good;
        }
        
    }
}

void
nsec3_zone_item_rrsig_delete_all(nsec3_zone_item *item)
{
    zdb_packed_ttlrdata *rrsig = item->rrsig;

    item->rrsig = NULL;

    while(rrsig != NULL)
    {
        zdb_packed_ttlrdata *tmp = rrsig;

        rrsig = rrsig->next;

        ZDB_RECORD_ZFREE(tmp);
    }
}

/*
 * Empties an nsec3_zone_item
 *
 * Only frees the payload : owners, stars, bitmap, rrsig
 * Does not change the other nodes of the structure
 *
 * This should be followed by the destruction of the item
 */

void
nsec3_zone_item_empties(nsec3_zone_item *item)
{
    nsec3_remove_all_star(item);
    nsec3_remove_all_owners(item);

    zassert(item->rc == 0 && item->sc == 0);

    ZFREE_ARRAY(item->type_bit_maps, item->type_bit_maps_size);

    item->type_bit_maps = NULL;
    item->type_bit_maps_size = 0;

    nsec3_zone_item_rrsig_delete_all(item);
}

/*
 * Sets the type bitmap of the nsec3 item to match the one in the rdata
 * Does nothing if the bitmap is already ok
 *
 * NOTE: Remember that the item does not contain
 *
 *  _ hash_algorithm
 *  _ iterations
 *  _ salt_length
 *  _ salt
 *  _ hash_length
 *  _ next_hashed_owner_name
 */

ya_result nsec3_zone_item_update_bitmap(nsec3_zone_item* nsec3_item, const u8 *rdata, u16 rdata_size)
{
    /*
     * Skip the irrelevant bytes
     */

    if(rdata_size < 8)
    {
        return ERROR;
    }

    const u8 *bitmap = rdata;
    u16 type_bit_maps_size = rdata_size;

    bitmap += 4;
    type_bit_maps_size -= 4;

    if(type_bit_maps_size < *bitmap + 1)
    {
        return ERROR;
    }

    type_bit_maps_size -= *bitmap + 1;
    bitmap += *bitmap + 1;

    if(type_bit_maps_size < *bitmap + 1)
    {
        return ERROR;
    }
    
    type_bit_maps_size -= *bitmap + 1;
    bitmap += *bitmap + 1;

    /*
     * If it does not match, replace.
     */

    if((nsec3_item->type_bit_maps_size != type_bit_maps_size) || (memcmp(nsec3_item->type_bit_maps, bitmap, type_bit_maps_size)==0))
    {
        /* If the size differs : free and alloc */

        if(nsec3_item->type_bit_maps_size != type_bit_maps_size)
        {
            /* TODO : take the memory granularty in account in case of ZALLOC enabled */
            ZFREE_ARRAY(nsec3_item->type_bit_maps, nsec3_item->type_bit_maps_size);
            ZALLOC_ARRAY_OR_DIE(u8*, nsec3_item->type_bit_maps, type_bit_maps_size, NSEC3_TYPEBITMAPS_TAG);
            nsec3_item->type_bit_maps_size = type_bit_maps_size;
        }

        memcpy(nsec3_item->type_bit_maps, bitmap, type_bit_maps_size);
    }

    return SUCCESS;
}

/** @} */

/*----------------------------------------------------------------------------*/

