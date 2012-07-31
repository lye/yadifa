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
 *
 *----------------------------------------------------------------------------*/
#ifndef _NSEC3_ITEM_H
#define	_NSEC3_ITEM_H

#include <dnsdb/nsec3_types.h>
#include <dnscore/output_stream.h>

#ifdef	__cplusplus
extern "C"
{
#endif

/* NOTE: The first byte of the digest is its length */
nsec3_zone_item* nsec3_zone_item_find(nsec3_zone *n3, const u8 *digest);

/**
 *
 * @param n3
 * @param dnsnamedigest
 * @return
 */
nsec3_zone_item* nsec3_zone_item_find_by_name(nsec3_zone *n3, const u8 *dnsnamedigest);

/**
 *
 * Also returns the nsec3 chain.
 *
 * @param zone
 * @param nsec3_label
 * @param out_n3
 * @return
 */
nsec3_zone_item* nsec3_zone_item_find_by_name_ext(zdb_zone *zone, const u8 *nsec3_label, nsec3_zone **out_n3);

nsec3_zone_item* nsec3_zone_item_find_by_record(zdb_zone *zone, const u8 *fqdn, u16 rdata_size, const u8 *rdata);

bool nsec3_zone_item_equals_rdata(nsec3_zone* n3,
                             nsec3_zone_item* item,
                             u16 rdata_size,
                             const u8* rdata);

void nsec3_zone_item_to_zdb_packed_ttlrdata(
			        nsec3_zone* n3,
			        nsec3_zone_item* item,
			        u8* origin,
			        u8* out_owner, /* dnsname */
                    u32 ttl,
			        zdb_packed_ttlrdata** out_nsec3,
			        zdb_packed_ttlrdata** out_nsec3_rrsig);

u32 nsec3_zone_item_get_label(   nsec3_zone_item* item,
                    u8* output_buffer,
                    u32 buffer_size);

void nsec3_zone_item_write_owner(  output_stream* os,
				    nsec3_zone_item* item,
				    u8* origin);

void nsec3_zone_item_to_output_stream(	output_stream* os,
					nsec3_zone* n3,
					nsec3_zone_item* item,
					u8* origin,
                    u32 ttl);

void nsec3_zone_item_rrsig_del_by_keytag(nsec3_zone_item *item, u16 native_key_tag);

void nsec3_zone_item_rrsig_del(nsec3_zone_item *item, zdb_ttlrdata *nsec3_rrsig);

void nsec3_zone_item_rrsig_add(nsec3_zone_item *item, zdb_packed_ttlrdata *nsec3_rrsig);

/*
 * Deletes ALL rrsig in the NSEC3 item
 */

void nsec3_zone_item_rrsig_delete_all(nsec3_zone_item *item);

/*
 * Empties an nsec3_zone_item
 *
 * Only frees the payload : owners, stars, bitmap, rrsig
 *
 * This should be followed by the destruction of the item itself
 */

void nsec3_zone_item_empties(nsec3_zone_item* item);

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

ya_result nsec3_zone_item_update_bitmap(nsec3_zone_item* nsec3_item, const u8 *rdata, u16 rdatasize);

#ifdef	__cplusplus
}
#endif

#endif	/* _NSEC3_ITEM_H */

/** @} */

/*----------------------------------------------------------------------------*/

