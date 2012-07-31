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
 *
 *----------------------------------------------------------------------------*/
#ifndef _NSEC_H
#define	_NSEC_H

#include <dnsdb/zdb_types.h>

#if ZDB_NSEC_SUPPORT == 0
#error "Please do not include nsec.h if ZDB_NSEC_SUPPORT is 0"
#endif

#include <dnsdb/nsec_collection.h>
#include <dnsdb/treeset.h>

#ifdef	__cplusplus
extern "C"
{
#endif

#define ZDB_NSECLABEL_TAG 0x4c42414c4345534e

#define NSEC_NEXT_DOMAIN_NAME(x__) (&(x__).rdata_start[0])

/*
struct nsec_label_extension
{
    nsec_node* node;
};
*/
struct nsec_zone
{
    nsec_label* last_nsec_label;
};

/**
 * Reverses the labels of the fqdn
 *
 * @param inverse_name
 * @param name
 * @return
 */

u32 nsec_inverse_name(u8 *inverse_name,const u8 *name);


ya_result nsec_update_zone(zdb_zone* zone);

/**
 * Creates the NSEC node, creates or update the NSEC record
 *
 * @param zone
 * @param label
 * @param labels
 * @param labels_top
 */

void nsec_update_label(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top);

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

bool nsec_update_label_record(zdb_rr_label *label, nsec_node *node, nsec_node *next_node, u8 *name, u32 ttl);

/**
 * Creates the NSEC node, link it to the label.
 *
 * @param zone
 * @param label
 * @param labels
 * @param labels_top
 * @return
 */

nsec_node *nsec_update_label_node(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top);

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

bool nsec_delete_label_node(zdb_zone* zone, zdb_rr_label* label, dnslabel_vector_reference labels, s32 labels_top);

/**
 *
 * Find the label that has got the right NSEC interval for "nextname"
 *
 * @param zone
 * @param name_vector
 * @param dname_out
 * @return
 */

zdb_rr_label *nsec_find_interval(const zdb_zone *zone, const dnsname_vector *name_vector, u8 *dname_out);

void nsec_name_error(const zdb_zone* zone, const dnsname_vector *qname_not_const, s32 closest_index,
                     u8* out_encloser_nsec_name,
                     zdb_rr_label** out_encloser_nsec_label,
                     u8* out_wild_encloser_nsec_name,
                     zdb_rr_label** out_wildencloser_nsec_label
                    );

void nsec_destroy_zone(zdb_zone* zone);

struct nsec_icmtl_replay
{
    treeset_tree nsec_del;
    treeset_tree nsec_add;
    zdb_zone *zone;
};

typedef struct nsec_icmtl_replay nsec_icmtl_replay;

/**
 * Initialises the replay structure
 *
 * @param replay
 * @param zone
 */

void nsec_icmtl_replay_init(nsec_icmtl_replay *replay, zdb_zone *zone);

void nsec_icmtl_replay_destroy(nsec_icmtl_replay *replay);

/**
 * Appends a NSEC del to the replay structure
 *
 * @param replay
 * @param fqdn
 * @param ttlrdata
 */
void nsec_icmtl_replay_nsec_del(nsec_icmtl_replay *replay, const u8* fqdn);

/**
 * Appends a NSEC add to the replay structure
 *
 * @param replay
 * @param fqdn
 * @param ttlrdata
 */
void nsec_icmtl_replay_nsec_add(nsec_icmtl_replay *replay, const u8* fqdn);

void nsec_icmtl_replay_execute(nsec_icmtl_replay *replay);

void nsec_logdump_tree(zdb_zone *zone);

#define ZONE_NSEC_AVAILABLE(zone_) ( ((zone_)->apex->flags & (ZDB_RR_LABEL_DNSSEC_EDIT|ZDB_RR_LABEL_NSEC)) == ZDB_RR_LABEL_NSEC)

#ifdef	__cplusplus
}
#endif

#endif	/* _NSEC_H */
/** @} */

/*----------------------------------------------------------------------------*/

