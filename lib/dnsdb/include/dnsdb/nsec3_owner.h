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
#ifndef _NSEC3_OWNER_H
#define	_NSEC3_OWNER_H

#include <dnsdb/nsec3_types.h>
#include <dnsdb/nsec3_collection.h>

#ifdef	__cplusplus
extern "C"
{
#endif

bool nsec3_owned_by(const nsec3_zone_item* item, const zdb_rr_label* owner);

/*
 * Adds an owner to the NSEC3 item
 */

void nsec3_add_owner(nsec3_zone_item* item, const zdb_rr_label* owner);

/*
 * Removes an owner from the NSEC3 item
 *
 * The entry MUST have been set before
 */

void nsec3_remove_owner(nsec3_zone_item* item, zdb_rr_label* owner);

/*
 * Removes all owners from the NSEC3 item
 *
 * The entry MUST have been set before
 */

void nsec3_remove_all_owners(nsec3_zone_item* item);

zdb_rr_label* nsec3_owner_get(nsec3_zone_item* item, u16 n);

static inline u16 nsec3_owner_count(nsec3_zone_item* item)
{
    return item->rc;
}

/*
 * Adds a "star" to the NSEC3 item
 */

void nsec3_add_star(nsec3_zone_item* item, const zdb_rr_label* owner);
/*
 * Removes a star from the NSEC3 item
 *
 * The entry MUST have been set before
 */

void nsec3_remove_star(nsec3_zone_item* item, zdb_rr_label* owner);

/*
 * Removes all stars from the NSEC3 item
 *
 * The entry MUST have been set before
 */

void nsec3_remove_all_star(nsec3_zone_item* item);

/*
 * Moves all stars from one NSEC3 item to another.
 *
 * This is used when an NSEC3 item is removed: All its NSEC3 must be moved
 * to his predecessor.
 */

void nsec3_move_all_star(nsec3_zone_item* src, nsec3_zone_item* dst);

zdb_rr_label* nsec3_star_get(const nsec3_zone_item* item, u16 n);

static inline u16 nsec3_star_count(nsec3_zone_item* item)
{
    return item->sc;
}

#ifdef	__cplusplus
}
#endif

#endif	/* _NSEC3_OWNER_H */
/** @} */

/*----------------------------------------------------------------------------*/

