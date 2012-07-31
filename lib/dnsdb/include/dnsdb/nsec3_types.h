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
#ifndef _NSEC3_TYPES_H
#define	_NSEC3_TYPES_H

#include <arpa/inet.h>

#include <dnscore/ptr_vector.h>
#include <dnsdb/zdb_types.h>
#include <dnsdb/nsec3_collection.h>

#if ZDB_NSEC3_SUPPORT == 0
#error "Please do not include nsec3.h if ZDB_NSEC3_SUPPORT is 0"
#endif

#ifdef	__cplusplus
extern "C"
{
#endif

/*
 * There is no sense in using more than one.
 * Two can be a transition state
 *
 * This limit is actually used for ICMTL generation
 * The NSEC3 structure is cheap on memory (every bit count on a TLD) but
 * there is a price for this.  It is mostly irrelevant, but for ICMTL and
 * anything trying to get specific NSEC3 rdata bits without knowing the
 * NSEC3PARAM as well.
 *
 */
#define MAX_SUPPORTED_NSEC3PARAM    4

#define NSEC3_DIGEST_TAG	    0x474944334e	/* N3DIG */
#define NSEC3_ZONE_TAG		    0x454e4f5a334e	/* N3ZONE */
#define NSEC3_CONTEXT_RECORD_TAG    0x585443334e	/* N3CTX */
#define NSEC3_RDATA_TAG		    0x4154414452334e	/* N3RDATA */
#define NSEC3_LABELEXT_TAG	    0x54584542414c334e	/* N3LABEXT */
#define NSEC3_TYPEBITMAPS_TAG	    0x5350414d4254334e	/* N3TBMAPS */
#define NSEC3_LABELPTRARRAY_TAG	    0x595252412a4c334e	/* N3L*ARRY */

    /** The NSEC3 node with this flag on is scheduled for a processing (ie: signature)
     *  It is thus FORBIDDEN to delete it (but it MUST be removed from the NSEC3 collection)
     *
     *  So instead of a delete the NSEC3_PROPRIETARY_FLAG_DELETED flag should be used and the NSEC3 record has to be put in a
     *  "scheduled for delete" list.  The schedule being done after the signature the nsec3 record will be effectively removed.
     *
     */

#define NSEC3_PROPRIETARY_FLAG_SCHEDULED 0x80
#define NSEC3_PROPRIETARY_FLAG_DELETED   0x40

//typedef struct nsec3_node nsec3_zone_item;
#define nsec3_zone_item struct nsec3_node

typedef nsec3_zone_item** nsec3_zone_item_pointer_array;

typedef struct nsec3_label_extension nsec3_label_extension;

typedef nsec3_label_extension** nsec3_label_extension_array;

/*
 * Index instead of a pointer.
 * The relevant information are:
 *
 * index
 * (index+1) MOD count
 */

struct nsec3_label_extension
{
    nsec3_zone_item* self;
    nsec3_zone_item* star;

    struct nsec3_label_extension *next;
};

/*
typedef struct nsec3_zone nsec3_zone;

 */

struct nsec3_zone
{
    struct nsec3_zone*	next;

    nsec3_zone_item* items;
    u8 rdata[1];
};

/*
 * This struct and the five functions are handling the loading of the nsec3
 * records of a zone file.
 *
 * This not a trivial task.
 */

struct nsec3_chain_context
{
    struct nsec3_chain_context *next;
    /*
    ptr_vector  nsec3;
    ptr_vector  rrsig;
    */
    u8         *nsec3param_rdata;
    u16         nsec3param_rdata_size;
};

typedef struct nsec3_chain_context nsec3_chain_context;

struct nsec3_load_context
{
    ptr_vector  nsec3;
    ptr_vector  rrsig;
    nsec3_chain_context *chain;
    zdb_zone* zone;
    
    u32 rrsig_added;
    u32 rrsig_ignored;
    u32 rrsig_discarded;
    
    u32 nsec3_accepted;
    u32 nsec3_rejected;
    u32 nsec3_discarded;
    
    bool opt_out;
};

typedef struct nsec3_load_context nsec3_load_context;


#define NSEC3_ZONE_FAKE_OWNER ((void*)~0)

#define NSEC3PARAM_MINIMUM_LENGTH		5

#define NSEC3PARAM_RDATA_ALGORITHM(n3prd)	((n3prd)[0])
#define NSEC3PARAM_RDATA_FLAGS(n3prd)		((n3prd)[1])
#define NSEC3PARAM_RDATA_SALT_LEN(n3prd)	((n3prd)[4])
#define NSEC3PARAM_RDATA_SALT(n3prd)		(&(n3prd)[NSEC3PARAM_MINIMUM_LENGTH])

#define NSEC3_ZONE_ALGORITHM(n3_)		NSEC3PARAM_RDATA_ALGORITHM((n3_)->rdata)
#define NSEC3_ZONE_FLAGS(n3_)			NSEC3PARAM_RDATA_FLAGS((n3_)->rdata)
#define NSEC3_ZONE_SALT_LEN(n3_)		NSEC3PARAM_RDATA_SALT_LEN((n3_)->rdata)
#define NSEC3_ZONE_SALT(n3_)	    		NSEC3PARAM_RDATA_SALT((n3_)->rdata)

#define NSEC3PARAM_DEFAULT_TTL			0
//#define NSEC3_DEFAULT_TTL			600                 /** @todo: Take it from the SOA instead */
//#define NSEC3_DEFAULT_FLAGS                     NSEC3_FLAGS_OPTOUT  /** @todo: Take it from a zone configration field */
#define NSEC3_FLAGS_MARKED_FOR_ICMTL_ADD		0x80   /* DO NOT PUT THIS IN THE RFC
							* IT'S PROPRIETARY
							*/

#define NSEC3_ZONE_RDATA_SIZE_FROM_SALT(salt_len) (NSEC3PARAM_MINIMUM_LENGTH + (salt_len))
#define NSEC3_ZONE_STRUCT_SIZE_FROM_SALT(salt_len) (sizeof(nsec3_zone) + NSEC3_ZONE_RDATA_SIZE_FROM_SALT(salt_len) - 1)

#define NSEC3_ZONE_RDATA_SIZE(n3_)		NSEC3_ZONE_RDATA_SIZE_FROM_SALT(NSEC3_ZONE_SALT_LEN(n3_))
#define NSEC3_ZONE_STRUCT_SIZE(n3_)		NSEC3_ZONE_STRUCT_SIZE_FROM_SALT(NSEC3_ZONE_SALT_LEN(n3_))

#define nsec3_zone_get_iterations(n3_)		(ntohs(GET_U16_AT((n3_)->rdata[2])))
#define nsec3_zone_set_iterations(n3_,iter_)	(GET_U16_AT((n3_)->rdata[2]) = htons(iter_))

#define nsec3_zone_get_item_next(n3_,idx_)	((nsec3_zone_item*)((n3_)->items.data[(idx_+1)%nsec3_zone_get_item_count(n3_)]))

#define ZONE_IS_NSEC3(zone_) (((zone_)->nsec.nsec3!=NULL) && (zdb_record_find(&(zone_)->apex->resource_record_set, TYPE_NSEC3PARAM)!=NULL))
#define ZONE_NSEC3_AVAILABLE(zone_) ( ((zone_)->apex->flags & (ZDB_RR_LABEL_DNSSEC_EDIT|ZDB_RR_LABEL_NSEC3)) == ZDB_RR_LABEL_NSEC3)

#ifdef	__cplusplus
}
#endif

#endif	/* _NSEC3_TYPES_H */
/** @} */

/*----------------------------------------------------------------------------*/

