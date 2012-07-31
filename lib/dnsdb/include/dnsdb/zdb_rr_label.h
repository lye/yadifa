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
/** @defgroup records_labels Internal functions for the database: zoned resource records label.
 *  @ingroup dnsdb
 *  @brief Internal functions for the database: zoned resource records label.
 *
 *  Internal functions for the database: zoned resource records label.
 *
 * @{
 */
#ifndef _RR_LABEL_H
#define	_RR_LABEL_H

#include "zdb_types.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define ZDB_RR_LABEL_DELETE_NODE     2          /* matches the dictionary delete node   */
#define ZDB_RR_LABEL_DELETE_TREE     3          /*                                      */

#define ZDB_RRLABEL_TAG 0x4c424c525242445a     /** "ZDBRRLBL" */
    
/**
 *  Instanciate a new rr_label.
 */

zdb_rr_label* zdb_rr_label_new_instance(const u8* label_name);

/**
 * @brief Finds the resource record label matching a path of labels starting from another rr label
 *
 * Finds the resource record label matching a path of labels starting from another rr label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label or NULL if it has not been found
 */

/* 3 USES */
zdb_rr_label* zdb_rr_label_find_exact(zdb_rr_label* apex,dnslabel_vector_reference path,s32 path_index);

/**
 * @brief Finds the resource record label matching a path of labels starting from another rr label or the wildcard label
 *
 * Finds the resource record label matching a path of labels starting from another rr label or the wildcard label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label, the * label or NULL if none of them has not been found
 */

/* NOT USED (YET) */
zdb_rr_label* zdb_rr_label_find(zdb_rr_label* apex, dnslabel_vector_reference sections, s32 index);

zdb_rr_label*
zdb_rr_label_stack_find(zdb_rr_label* apex, dnslabel_stack_reference sections, s32 pos, s32 index);

zdb_rr_label* zdb_rr_label_find_child(zdb_rr_label* parent, const u8* dns_label);

/**
 * @brief Finds the closest resource record label matching a path of labels starting from another rr label
 *
 * Finds the resource record label matching a path of labels starting from another rr label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[inout] path_index the index of the top of the stack, set the index of the label in the stack at return
 *
 * @return the matching label or the apex if none has been found
 */

/* 3 USES */
zdb_rr_label* zdb_rr_label_find_closest(zdb_rr_label* apex, dnslabel_vector_reference path, s32* closest_level);

zdb_rr_label* zdb_rr_label_find_closest_authority(zdb_rr_label* apex, dnslabel_vector_reference path, s32* closest_level);

struct zdb_rr_label_find_ext_data
{
    zdb_rr_label *authority;
    zdb_rr_label *closest;
    zdb_rr_label *answer;
    s32 authority_index;
    s32 closest_index;
};

typedef struct zdb_rr_label_find_ext_data zdb_rr_label_find_ext_data;

zdb_rr_label* zdb_rr_label_find_ext(zdb_rr_label* apex, dnslabel_vector_reference sections, s32 index_, zdb_rr_label_find_ext_data *ext);

/**
 * @brief Returns the authority fqdn
 * 
 * Returns the pointer to the authority fqdn located inside the qname (based on the rr_label_info)
 * 
 * @param qname
 * @param rr_label_info
 * @return 
 */

static inline const u8 *
zdb_rr_label_info_get_authority_qname(const u8 *qname, const zdb_rr_label_find_ext_data *rr_label_info)
{
    const u8 * authority_qname = qname;

    s32 i = rr_label_info->authority_index;
    while(i > 0)
    {
        authority_qname += authority_qname[0] + 1;
        i--;
    }
    
    return authority_qname;
}

/**
 * @brief Adds the resource record label matching a path of labels starting from another rr label
 *
 * Adds the resource record label matching a path of labels starting from another rr label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label or NULL if it has not been found
 */

/* 1 USE */
zdb_rr_label* zdb_rr_label_add(zdb_zone* zone, dnslabel_vector_reference sections, s32 index);

/**
 * @brief Deletes the resource record of the given type on the label matching a path of labels starting from another rr label
 *
 * Deletes the resource record of the given type on the label matching a path of labels starting from another rr label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label or NULL if it has not been found
 */

/* 2 USES */
ya_result zdb_rr_label_delete_record(zdb_zone* zone, dnslabel_vector_reference path, s32 path_index, u16 type);

/**
 * @brief Deletes the resource record of the given type, ttl and rdata on the label matching a path of labels starting from another rr label
 *
 * Deletes the resource record of the given type, ttl and rdata on the label matching a path of labels starting from another rr label
 * Typically the starting label is a zone cut.
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label or NULL if it has not been found
 */

/* 1 USE */
ya_result zdb_rr_label_delete_record_exact(zdb_zone* zone,dnslabel_vector_reference path,s32 path_index,u16 type,zdb_ttlrdata* ttlrdata);

/**
 * @brief Deletes an EMPTY label (TODO)
 *
 * Deletes an EMPTY label an all it's EMPTY parents
 * Parents bound to an NSEC structures are not removed
 *
 * @param[in] apex the starting label
 * @param[in] path a stack of labels
 * @param[in] path_index the index of the top of the stack
 *
 * @return the matching label or NULL if it has not been found
 */

/* 1 USE */
ya_result zdb_rr_label_delete(zdb_zone* zone,dnslabel_vector_reference path,s32 path_index);


/**
 * @brief Destroys a zone label and its contents
 *
 * Destroys a zone label and its contents
 *
 * @param[in] zone_labep a pointer to a pointer to the label to destroy
 *
 */

/* 1 USE */
void zdb_rr_label_destroy(zdb_zone* zone, zdb_rr_label** rr_labelp);

void zdb_rr_label_truncate(zdb_zone* zone, zdb_rr_label* rr_labelp);

static inline bool zdb_rr_label_is_glue(zdb_rr_label* label)
{
    return (label->flags & (ZDB_RR_LABEL_UNDERDELEGATION | ZDB_RR_LABEL_DELEGATION)) == ZDB_RR_LABEL_UNDERDELEGATION;
}

#if ZDB_DNSSEC_SUPPORT != 0
/* 2 USES */
#define RR_LABEL_RELEVANT(rr_label_)   ((dictionary_notempty(&(rr_label_)->sub))||(btree_notempty((rr_label_)->resource_record_set))||(rr_label_->nsec.dnssec != NULL))

/* label is only alive because of NSEC3 */
#define RR_LABEL_HASSUB(rr_label_)      (dictionary_notempty(&(rr_label_)->sub))
#define RR_LABEL_HASSUBORREC(rr_label_)((dictionary_notempty(&(rr_label_)->sub))||(btree_notempty((rr_label_)->resource_record_set)))
#define RR_LABEL_NOSUBNORREC(rr_label_)((dictionary_notempty(&(rr_label_)->sub))&&(btree_notempty((rr_label_)->resource_record_set)))

/* 9 USES */
#define RR_LABEL_IRRELEVANT(rr_label_) ((dictionary_isempty(&(rr_label_)->sub))&&(btree_isempty((rr_label_)->resource_record_set))&&(rr_label_->nsec.dnssec == NULL))

#else
/* 2 USES */
#define RR_LABEL_RELEVANT(rr_label_)   (dictionary_notempty(&(rr_label_)->sub)||btree_notempty(&(rr_label_)->resource_record_set))

/* 9 USES */
#define RR_LABEL_IRRELEVANT(rr_label_) (dictionary_isempty(&(rr_label_)->sub)&&btree_isempty(&(rr_label_)->resource_record_set))

#endif

/* 0 USES */
#define RR_LABEL_HASRECORDS(rr_label_) btree_isempty(&(rr_label_)->resource_record_set)

#ifndef NDEBUG

void zdb_rr_label_print_indented(zdb_rr_label* rr_label, int indent);

void zdb_rr_label_print(zdb_rr_label* zone_label);

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _rr_LABEL_H */

/** @} */
