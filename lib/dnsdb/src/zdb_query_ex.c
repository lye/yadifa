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
/** @defgroup query_ex Database top-level query function
 *  @ingroup dnsdb
 *  @brief Database top-level query function
 *
 *  Database top-level query function
 *
 * @{
 */

#include <stdio.h>
#include <stdlib.h>

#define DEBUG_LEVEL 0

#include <dnscore/dnscore.h>
#include <dnscore/format.h>
#include <dnscore/random.h>
#include <dnscore/dnsname_set.h>

#include <dnscore/rdtsc.h>

#include "dnsdb/zdb.h"
#include "dnsdb/zdb_zone.h"
#include "dnsdb/zdb_zone_label.h"
#include "dnsdb/zdb_rr_label.h"
#include "dnsdb/zdb_record.h"
#include "dnsdb/zdb_dnsname.h"
#include "dnsdb/dictionary.h"

#include <dnscore/message.h>

#if ZDB_NSEC_SUPPORT != 0
#include "dnsdb/nsec.h"
#endif

#if ZDB_NSEC3_SUPPORT != 0
#include "dnsdb/nsec3.h"
#endif

#if ZDB_DNSSEC_SUPPORT != 0
#include "dnsdb/rrsig.h"
#endif

/**
 * In order to optimise-out the class parameter that is not required if ZDB_RECORDS_MAX_CLASS == 1 ...
 */

#if ZDB_RECORDS_MAX_CLASS != 1
#define DECLARE_ZCLASS_PARAMETER    u16 zclass,
#define PASS_ZCLASS_PARAMETER       zclass,
#define PASS_ZONE_ZCLASS_PARAMETER  zone->zclass,
#else
#define DECLARE_ZCLASS_PARAMETER
#define PASS_ZCLASS_PARAMETER
#define PASS_ZONE_ZCLASS_PARAMETER
#endif     

extern logger_handle* g_database_logger;
#define MODULE_MSG_HANDLE g_database_logger

/** @brief Creates a answer node from a database record
 *
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param pool the memory pool
 * 
 * @return a resource record suitable for network serialisation
 *
 * 5 uses
 * 
 */

static inline zdb_resourcerecord*
zdb_query_ex_answer_make(const zdb_packed_ttlrdata* source, const u8* name,
                         DECLARE_ZCLASS_PARAMETER
                         u16 rtype, u8 * restrict * pool)
{
    zassert(source != NULL && name != NULL);

    zdb_resourcerecord* node = (zdb_resourcerecord*) * pool;

#ifndef NDEBUG
    memset(node, 0xff, ALIGN16(sizeof(zdb_resourcerecord)));
#endif

    *pool += ALIGN16(sizeof(zdb_resourcerecord));

    node->next = NULL;
    node->ttl_rdata = (zdb_packed_ttlrdata*)source;
    /** @note I should not need to clone the name
     *  It comes either from the query, either from an rdata in the database.
     */
    node->name = name;
    
#if ZDB_RECORDS_MAX_CLASS != 1
    node->zclass = zclass;
#else
    node->zclass = CLASS_IN;
#endif
    
    node->rtype = rtype;
    node->ttl = source->ttl;

    return node;
}

/** @brief Creates a answer node from a database record with a specific TTL
 *
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param ttl the TTL that replaces the one in the record
 * @param pool the memory pool
 *
 * @return a resource record suitable for network serialisation
 * 
 * 5 uses
 */

static inline zdb_resourcerecord*
zdb_query_ex_answer_make_ttl(const zdb_packed_ttlrdata* source, const u8* name,
                             DECLARE_ZCLASS_PARAMETER
                             u16 rtype, u32 ttl, u8 * restrict * pool)
{
    zassert(source != NULL && name != NULL);

    zdb_resourcerecord* node = (zdb_resourcerecord*) * pool;

#ifndef NDEBUG
    memset(node, 0xff, ALIGN16(sizeof(zdb_resourcerecord)));
#endif

    *pool += ALIGN16(sizeof(zdb_resourcerecord));

    node->next = NULL;
    node->ttl_rdata = (zdb_packed_ttlrdata*)source;
    /** @note I should not need to clone the name
     *  It comes either from the query, either from an rdata in the database.
     */
    
    node->name = name;
    
#if ZDB_RECORDS_MAX_CLASS != 1
    node->zclass = zclass;
#else
    node->zclass = CLASS_IN;
#endif
    
    node->rtype = rtype;

    node->ttl = ttl;

    return node;
}

/** @brief Appends a list of database records to a list of nodes at a random position
 *
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * 6 uses
 */
static void
zdb_query_ex_answer_appendrndlist(const zdb_packed_ttlrdata* source, const u8* label,
                                  DECLARE_ZCLASS_PARAMETER
                                  u16 type, zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(source != NULL && label != NULL);

    zdb_resourcerecord* head = zdb_query_ex_answer_make(source, label,
                                                        PASS_ZCLASS_PARAMETER
                                                        type, pool);
    head->next = *headp;
    source = source->next;

    if(source != NULL)
    {
        random_ctx rndctx = thread_pool_get_random_ctx();

        int rnd = random_next(rndctx);

        do
        {
            zdb_resourcerecord* node = zdb_query_ex_answer_make(source, label,
                                            PASS_ZCLASS_PARAMETER
                                            type, pool);

            if(rnd & 1)
            {
                /* put the new node in front of the head,
                 * and assign the head to node
                 */

                node->next = head;
                head = node;
            }
            else
            {
                /* put the new node next to the head */
                node->next = head->next;
                head->next = node;
            }

            rnd >>= 1;

            /**
             *  @note: After 32 entries it will not be so randomized anymore ...
             */

            source = source->next;
        }
        while(source != NULL);
    }

    *headp = head;
}

/** @brief Appends a list of database records to a list of nodes
 *
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param headp a pointer to the section list
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * 10 uses
 */
static void
zdb_query_ex_answer_appendlist(const zdb_packed_ttlrdata* source, const u8* label,
                               DECLARE_ZCLASS_PARAMETER
                               u16 rtype, zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(source != NULL && label != NULL);

    zdb_resourcerecord* head = *headp;
    while(source != NULL)
    {
        zdb_resourcerecord* node = zdb_query_ex_answer_make(source, label, PASS_ZCLASS_PARAMETER rtype, pool);
        
        node->next = head;
        head = node;
        source = source->next;
    }
    *headp = head;
}

/** @brief Appends a list of database records to a list of nodes with a specific TTL
 *
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param ttl the ttl of the record set
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * 12 uses
 */
static void
zdb_query_ex_answer_appendlist_ttl(const zdb_packed_ttlrdata* source, const u8* label,
                                   DECLARE_ZCLASS_PARAMETER
                                   u16 rtype, u32 ttl, zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(source != NULL && label != NULL);

    zdb_resourcerecord* next = *headp;
    while(source != NULL)
    {
        zdb_resourcerecord* node = zdb_query_ex_answer_make_ttl(source, label, 
                                                                PASS_ZCLASS_PARAMETER                                                                
                                                                rtype, ttl, pool);
        node->next = next;
        next = node;
        source = source->next;
    }
    *headp = next;
}

/** @brief Appends a list of database records to a list of nodes
 *
 * At the end
 * 
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * 5 uses
 */
static void
zdb_query_ex_answer_append(const zdb_packed_ttlrdata* source, const u8* label,
                           DECLARE_ZCLASS_PARAMETER
                           u16 type, zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(source != NULL);
    zassert(label != NULL);

    zdb_resourcerecord* next = *headp;
    zdb_resourcerecord* head = zdb_query_ex_answer_make(source, label,
                                                        PASS_ZCLASS_PARAMETER
                                                        type, pool);
    if(next != NULL)
    {
        while(next->next != NULL)
        {
            next = next->next;
        }
        next->next = head;
    }
    else
    {
        *headp = head;
    }
}

/** @brief Appends a list of database records to a list of nodes with a specific TTL
 *
 * At the end
 * 
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param rtype the type of the record
 * @param ttl the ttl of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * 16 uses
 */
static void
zdb_query_ex_answer_append_ttl(const zdb_packed_ttlrdata* source, const u8* label,
                               DECLARE_ZCLASS_PARAMETER
                               u16 rtype, u32 ttl, zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(source != NULL);
    zassert(label != NULL);

    zdb_resourcerecord* next = *headp;
    zdb_resourcerecord* head = zdb_query_ex_answer_make_ttl(source, label, 
                                                            PASS_ZCLASS_PARAMETER
                                                            rtype, ttl, pool);
    if(next != NULL)
    {
        while(next->next != NULL)       /* look for the last node */
        {
            next = next->next;
        }
        next->next = head;              /* set the value */
    }
    else
    {
        *headp = head;                  /* set the head */
    }
}

/** @brief Appends an RRSIG record set to a list of nodes with a specific TTL
 *
 * At the end
 * 
 * @param source a pointer to the ttlrdata to put into the node
 * @param name the owner of the record
 * @param zclass (if more than one class is supported in the database)
 * @param ttl the ttl of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 *
 * @return a resource record suitable for network serialisation
 * 
 * 2 uses
 */
static inline void
zdb_query_ex_answer_append_rrsig(const zdb_packed_ttlrdata *source, const u8 *label,
                                 DECLARE_ZCLASS_PARAMETER
                                 u32 ttl, zdb_resourcerecord **headp, u8 * restrict * pool)
{
    zdb_query_ex_answer_append_ttl(source, label,
                                   PASS_ZCLASS_PARAMETER
                                   TYPE_RRSIG, ttl, headp, pool);
}

/** @brief Appends the RRSIG rrset that covers the given type
 *
 * At the end
 * 
 * @param label the database label that owns the rrset
 * @param label_fqdn the owner of the records
 * @param zclass (if more than one class is supported in the database)
 * @param ttl the ttl of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 20 uses
 */
static void
zdb_query_ex_answer_append_type_rrsigs(const zdb_rr_label *label, const u8 *label_fqdn, u16 rtype,
                                       DECLARE_ZCLASS_PARAMETER
                                       u32 ttl, zdb_resourcerecord **headp, u8 * restrict * pool)
{
    const zdb_packed_ttlrdata *type_rrsig = rrsig_find(label, rtype);

    while(type_rrsig != NULL)
    {
        zdb_query_ex_answer_append_rrsig(type_rrsig, label_fqdn, 
                                         PASS_ZCLASS_PARAMETER
                                         ttl, headp, pool);

        type_rrsig = rrsig_next(type_rrsig, rtype);
    }
}

/** @brief Appends the RRSIG rrset that covers the given type
 *
 * At the end
 * 
 * @param rrsig_list an RRSIG rrset to take the signatures from
 * @param label_fqdn the owner of the records
 * @param zclass (if more than one class is supported in the database)
 * @param ttl the ttl of the record
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 2 uses
 */
static void
zdb_query_ex_answer_append_type_rrsigs_from(const zdb_packed_ttlrdata *rrsig_list, const u8 *label_fqdn, u16 rtype,
                                            DECLARE_ZCLASS_PARAMETER
                                            u32 ttl, zdb_resourcerecord **headp, u8 * restrict * pool)
{
    const zdb_packed_ttlrdata *rrsig = rrsig_list;

    do
    {
        if(RRSIG_TYPE_COVERED(rrsig) == rtype)
        {
            zdb_query_ex_answer_append_rrsig(rrsig, label_fqdn, 
                                             PASS_ZCLASS_PARAMETER
                                             ttl, headp, pool);
        }

        rrsig = rrsig->next;
    } 
    while(rrsig != NULL);
}

/** @brief Appends the NSEC interval for the given name
 *
 * At the end
 * 
 * @param zone the zone
 * @param name the name path
 * @param dups the label that cannot be added (used for wildcards)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 3 uses
 */
static void
zdb_query_ex_add_nsec_interval(const zdb_zone *zone, const dnsname_vector* name,
                               zdb_rr_label* dups, zdb_resourcerecord** headp,
                               u8 * restrict * pool)
{
    zdb_rr_label *nsec_interval_label;

    u8* nsec_dnsname = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u32 min_ttl;
    
    zdb_zone_getminttl(zone, &min_ttl);

    nsec_interval_label = nsec_find_interval(zone, name, nsec_dnsname);
    
    zassert(nsec_interval_label != NULL);

    if(/*(nsec_interval_label != NULL) && */(nsec_interval_label != dups))
    {
        zdb_packed_ttlrdata *nsec_interval_label_nsec = zdb_record_find(&nsec_interval_label->resource_record_set, TYPE_NSEC);

        if(nsec_interval_label_nsec != NULL)
        {
            zdb_packed_ttlrdata *nsec_interval_label_nsec_rrsig = rrsig_find(nsec_interval_label, TYPE_NSEC);
            
            if(nsec_interval_label_nsec_rrsig != NULL)
            {
                zdb_query_ex_answer_append_ttl(nsec_interval_label_nsec, nsec_dnsname,
                                            PASS_ZONE_ZCLASS_PARAMETER
                                            TYPE_NSEC, min_ttl, headp, pool);
                do
                {
                    zdb_query_ex_answer_append_ttl(nsec_interval_label_nsec_rrsig, nsec_dnsname,
                                                PASS_ZONE_ZCLASS_PARAMETER
                                                TYPE_RRSIG, min_ttl, headp, pool);

                    nsec_interval_label_nsec_rrsig = rrsig_next(nsec_interval_label_nsec_rrsig, TYPE_NSEC);
                }
                while(nsec_interval_label_nsec_rrsig != NULL);
            }
        }
    }
}

/** @brief Appends the SOA negative ttl record
 *
 * At the end
 * 
 * @param zone the zone
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 3 uses
 */
static void
zdb_query_ex_answer_append_soa_nttl(const zdb_zone *zone, zdb_resourcerecord **headp,u8 * restrict * pool)
{
    zassert(zone != NULL);
    
    const u8* label = zone->origin;
#if ZDB_RECORDS_MAX_CLASS != 1
    u16 zclass = zone->zclass;
#endif
    zdb_rr_collection* apex_records = &zone->apex->resource_record_set;
    zdb_packed_ttlrdata* zone_soa = zdb_record_find(apex_records, TYPE_SOA);

    zdb_resourcerecord* next = *headp;

    u32 ttl;
    zdb_zone_getminttl(zone, &ttl);
    
    zdb_resourcerecord* node = zdb_query_ex_answer_make_ttl(zone_soa, label,
                                                            PASS_ZCLASS_PARAMETER
                                                            TYPE_SOA, ttl, pool);

    if(next != NULL)
    {
        while(next->next != NULL)
        {
            next = next->next;
        }
        next->next = node;
    }
    else
    {
        *headp = node;
    }
}

/** @brief Appends the SOA negative ttl record and its signature
 *
 * At the end
 * 
 * @param zone the zone
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * @returns the minimum TTL (OBSOLETE !)
 * 
 * 3 uses
 */

static u32
zdb_query_ex_answer_append_soa_rrsig_nttl(const zdb_zone *zone, zdb_resourcerecord **headp, u8 * restrict * pool)
{
    zassert(zone != NULL);

    const u8* label_fqdn = zone->origin;
#if ZDB_RECORDS_MAX_CLASS != 1
    u16 zclass = zone->zclass;
#endif
    zdb_rr_collection* apex_records = &zone->apex->resource_record_set;
    zdb_packed_ttlrdata* zone_soa = zdb_record_find(apex_records, TYPE_SOA);
    
    zdb_resourcerecord* next = *headp;

    u32 ttl;
    zdb_zone_getminttl(zone, &ttl);
    
    zdb_resourcerecord* node = zdb_query_ex_answer_make_ttl(zone_soa, label_fqdn, 
                                                            PASS_ZCLASS_PARAMETER
                                                            TYPE_SOA, ttl, pool);
    if(next != NULL)
    {
        while(next->next != NULL)
        {
            next = next->next;
        }
        next->next = node;
    }
    else
    {
        *headp = node;
    }
    
    zdb_query_ex_answer_append_type_rrsigs(zone->apex, label_fqdn, TYPE_SOA, 
                                           PASS_ZCLASS_PARAMETER
                                           ttl, headp, pool);
    
    return ttl;
}

/**
 * @brief Returns the label for the dns_name, relative to the apex of the zone
 * 
 * @param zone the zone
 * @param dns_name the name of the label to find
 * 
 * @return a pointer the label
 * 
 * 2 uses
 */

static zdb_rr_label*
zdb_query_rr_label_find_relative(const zdb_zone* zone, const u8* dns_name)
{
    /*
     * Get the relative path
     */

    const dnslabel_vector_reference origin = (const dnslabel_vector_reference)zone->origin_vector.labels;
    s32 origin_top = zone->origin_vector.size;

    dnslabel_vector name;
    s32 name_top = dnsname_to_dnslabel_vector(dns_name, name);

    s32 i;

    for(i = 0; i <= origin_top; i++)
    {
        if(!dnslabel_equals(origin[origin_top - i], name[name_top - i]))
        {
            return NULL;
        }
    }

    /*
     * At this point we got the relative path, get the label
     *
     */

    zdb_rr_label* rr_label = zdb_rr_label_find(zone->apex, name, (name_top - origin_top) - 1);

    return rr_label;
}

/**
 * @brief Appends all the IPs (A & AAAA) under a name on the given zone
 * 
 * @param zone the zone
 * @param dns_name the name of the label to find
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * @param dnssec dnssec enabled or not
 * 
 * 1 use
 */

static inline void
zdb_query_ex_answer_append_ips(const zdb_zone* zone, const u8* dns_name,
                               DECLARE_ZCLASS_PARAMETER
                               zdb_resourcerecord** headp, u8 * restrict * pool, bool dnssec)
{
    /* Find relatively from the zone */
    zassert(dns_name != NULL);

    zdb_rr_label* rr_label = zdb_query_rr_label_find_relative(zone, dns_name);

    if(rr_label != NULL)
    {
        /* Get the label, instead of the type in the label */
        zdb_packed_ttlrdata* a = zdb_record_find(&rr_label->resource_record_set, TYPE_A);

        if(a != NULL)
        {
            zdb_query_ex_answer_appendlist(a, dns_name, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_A, headp, pool);

#if ZDB_DNSSEC_SUPPORT != 0
            if(dnssec)
            {
                zdb_query_ex_answer_append_type_rrsigs(rr_label, dns_name, TYPE_A, 
                                                       PASS_ZCLASS_PARAMETER
                                                       a->ttl, headp, pool);
            }
#endif
        }

        zdb_packed_ttlrdata* aaaa = zdb_record_find(&rr_label->resource_record_set, TYPE_AAAA);

        if(aaaa != NULL)
        {
            zdb_query_ex_answer_appendlist(aaaa, dns_name, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_AAAA, headp, pool);

#if ZDB_DNSSEC_SUPPORT != 0
            if(dnssec)
            {
                zdb_query_ex_answer_append_type_rrsigs(rr_label, dns_name, TYPE_AAAA, 
                                                       PASS_ZCLASS_PARAMETER
                                                       aaaa->ttl, headp, pool);
            }
#endif
        }
    }
}

/**
 * @brief Update a name set with the name found in an RDATA
 * 
 * @param source the record rdata containing the name to add
 * @param headp a pointer to the section list
 * @param rtype the type of the record
 * @param set collection where to add the name
 * 
 * 10 use
 */
static void
update_additionals_dname_set(const zdb_packed_ttlrdata* source,
                             DECLARE_ZCLASS_PARAMETER
                             u16 rtype, dnsname_set* set)
{
    if(source == NULL)
    {
        return;
    }

    u32 offset = 0;

    switch(rtype)
    {
        case TYPE_MX:
        {
            offset = 2;

            /* fallthrough */
        }
        case TYPE_NS:
        {
            do
            {
                /* ADD NS "A/AAAA" TO ADDITIONAL  */

                const u8 *dns_name = ZDB_PACKEDRECORD_PTR_RDATAPTR(source);
                dns_name += offset;

                dnsname_set_insert(set, dns_name);

                source = source->next;
            }
            while(source != NULL);

            break;
        }
    }
}

/**
 * @brief Update a name set with the name found in an RDATA
 * 
 * @param zone
 * @param zclass (if more than one class is supported in the database)
 * @param set collection where to add the name
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * @param dnssec dnssec enabled or not
 * 
 * 10 use
 */
static void
append_additionals_dname_set(const zdb_zone* zone,
                             DECLARE_ZCLASS_PARAMETER
                             dnsname_set* set, zdb_resourcerecord** headp, u8 * restrict * pool, bool dnssec)
{
    dnsname_set_iterator iter;

    dnsname_set_iterator_init(set, &iter);

    while(dnsname_set_iterator_hasnext(&iter))
    {
        /* ADD NS "A/AAAA" TO ADDITIONAL  */

        const u8* dns_name = dnsname_set_iterator_next_node(&iter)->key;

        zdb_query_ex_answer_append_ips(zone, dns_name,
                                       PASS_ZCLASS_PARAMETER
                                       headp, pool, dnssec);
    }
}

/**
 * @brief Appends NS records to a section
 * 
 * Appends NS records from the label to the referenced section
 * Also appends RRSIG for these NS
 * 
 * @param qname
 * @param rr_label_info
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * @param dnssec dnssec enabled or not
 * 
 * 3 uses
 */
static zdb_packed_ttlrdata*
append_authority(const u8 * qname,
                 DECLARE_ZCLASS_PARAMETER
                 const zdb_rr_label_find_ext_data* rr_label_info, zdb_resourcerecord** headp, u8 * restrict * pool, bool dnssec)
{
    zdb_packed_ttlrdata* authority = zdb_record_find(&rr_label_info->authority->resource_record_set, TYPE_NS);

    if(authority != NULL)
    {
        s32 i = rr_label_info->authority_index;
        
        while(i > 0)
        {
            qname += qname[0] + 1;
            i--;
        }

        zdb_query_ex_answer_appendrndlist(authority, qname, 
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NS, headp, pool);

#if ZDB_DNSSEC_SUPPORT != 0
        if(dnssec)
        {
            zdb_query_ex_answer_append_type_rrsigs(rr_label_info->authority, qname, TYPE_NS, 
                                                   PASS_ZCLASS_PARAMETER
                                                   authority->ttl, headp, pool);
            
            zdb_packed_ttlrdata* dsset = zdb_record_find(&rr_label_info->authority->resource_record_set, TYPE_DS);
            
            if(dsset != NULL)
            {
                zdb_query_ex_answer_appendlist(dsset, qname, 
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_DS, headp, pool);                
                zdb_query_ex_answer_append_type_rrsigs(rr_label_info->authority, qname, TYPE_DS, 
                                                       PASS_ZCLASS_PARAMETER
                                                       dsset->ttl, headp, pool);
            }
            
        }
#endif
    }

    return authority;
}

#if ZDB_NSEC3_SUPPORT != 0

/**
 * @brief Appends the NSEC3 - NODATA answer to the section
 * 
 * @param zone the zone
 * @param rr_label the covered label
 * @param name the owner name
 * @param apex_index the index of the apex in the name
 * @param type the type of record required
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 2 uses
 */
static inline void
zdb_query_ex_append_nsec3_nodata(const zdb_zone *zone, const zdb_rr_label *rr_label,
                                 const dnsname_vector *name, s32 apex_index, u16 rtype,
                                 DECLARE_ZCLASS_PARAMETER
                                 zdb_resourcerecord** headp, u8 * restrict * pool)
{
    nsec3_zone* n3 = zone->nsec.nsec3;

    u8 *nsec3_owner = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u8 *closest_nsec3_owner = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
#ifndef NDEBUG
    memset(nsec3_owner, 0xff, MAX_DOMAIN_LENGTH);
    memset(closest_nsec3_owner, 0xff, MAX_DOMAIN_LENGTH);
#endif
    
    u32 min_ttl;
    zdb_zone_getminttl(zone, &min_ttl);

    zdb_packed_ttlrdata* nsec3;
    zdb_packed_ttlrdata* nsec3_rrsig;
    zdb_packed_ttlrdata* closest_nsec3;
    zdb_packed_ttlrdata* closest_nsec3_rrsig;
    
    if(!IS_WILD_LABEL(rr_label->name))
    {
        if(rtype != TYPE_DS)
        {
            nsec3_nodata_error(zone, rr_label, name, apex_index, nsec3_owner, &nsec3, &nsec3_rrsig, closest_nsec3_owner, &closest_nsec3, &closest_nsec3_rrsig);
        }
        else
        {   
            closest_nsec3 = NULL;
            closest_nsec3_rrsig = NULL;
            
            if((rr_label->nsec.dnssec != NULL))
            {
                nsec3_zone_item *owner_nsec3 = rr_label->nsec.nsec3->self;
                nsec3_zone *n3 = zone->nsec.nsec3;
                
                if(owner_nsec3 != NULL)
                {
                    nsec3_zone_item_to_zdb_packed_ttlrdata(n3,
                                           owner_nsec3,
                                           zone->origin,
                                           nsec3_owner,
                                           min_ttl,
                                           &nsec3,
                                           &nsec3_rrsig);                    
                }
            }
            else
            {
                u8 *wild_closest_nsec3_owner = *pool;
                *pool += ALIGN16(MAX_DOMAIN_LENGTH);

                zdb_packed_ttlrdata* wild_closest_nsec3;
                zdb_packed_ttlrdata* wild_closest_nsec3_rrsig;

#ifndef NDEBUG
                memset(wild_closest_nsec3_owner, 0xff, sizeof (nsec3_owner));
#endif
                /* closest encloser proof */
                nsec3_wild_nodata_error(zone, rr_label, name, apex_index, nsec3_owner, &nsec3, &nsec3_rrsig, closest_nsec3_owner, &closest_nsec3, &closest_nsec3_rrsig, wild_closest_nsec3_owner, &wild_closest_nsec3, &wild_closest_nsec3_rrsig);

                if((closest_nsec3 != NULL) && (closest_nsec3_rrsig != NULL))
                {
#ifndef NDEBUG
                    log_debug("zdb_query_ex: nsec3_nodata_error: closest_nsec3_owner: %{dnsname}", closest_nsec3_owner);
#endif
                    zdb_query_ex_answer_append_ttl(closest_nsec3, closest_nsec3_owner, 
                                                   PASS_ZCLASS_PARAMETER
                                                   TYPE_NSEC3, min_ttl, headp, pool);
                    
                    zdb_query_ex_answer_appendlist_ttl(closest_nsec3_rrsig, closest_nsec3_owner, 
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_RRSIG, min_ttl, headp, pool);
                    
                    if(NSEC3_RDATA_IS_OPTOUT(ZDB_PACKEDRECORD_PTR_RDATAPTR(closest_nsec3)) && (nsec3 != NULL) && (nsec3_rrsig != NULL))
                    {
#ifndef NDEBUG
                        log_debug("zdb_query_ex: nsec3_nodata_error: nsec3_owner: %{dnsname}", nsec3_owner);
#endif
                        zdb_query_ex_answer_append_ttl(nsec3, nsec3_owner, 
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_NSEC3, min_ttl, headp, pool);

                        zdb_query_ex_answer_appendlist_ttl(nsec3_rrsig, nsec3_owner, 
                                                           PASS_ZCLASS_PARAMETER
                                                           TYPE_RRSIG, min_ttl, headp, pool);
                    }
                }
                
                return;
            }
        }
    }
    else
    {
        u8 *wild_closest_nsec3_owner = *pool;
        *pool += ALIGN16(MAX_DOMAIN_LENGTH);
        
        zdb_packed_ttlrdata* wild_closest_nsec3;
        zdb_packed_ttlrdata* wild_closest_nsec3_rrsig;
        
#ifndef NDEBUG
        memset(wild_closest_nsec3_owner, 0xff, sizeof (nsec3_owner));
#endif

        nsec3_wild_nodata_error(zone, rr_label, name, apex_index, nsec3_owner, &nsec3, &nsec3_rrsig, closest_nsec3_owner, &closest_nsec3, &closest_nsec3_rrsig, wild_closest_nsec3_owner, &wild_closest_nsec3, &wild_closest_nsec3_rrsig);
        
        if((wild_closest_nsec3 != NULL) && (wild_closest_nsec3_rrsig != NULL))
        {
#ifndef NDEBUG
            log_debug("zdb_query_ex: nsec3_nodata_error: closest_nsec3_owner: %{dnsname}", closest_nsec3_owner);
#endif
            zdb_query_ex_answer_append_ttl(wild_closest_nsec3, wild_closest_nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_NSEC3, min_ttl, headp, pool);
            zdb_query_ex_answer_appendlist_ttl(wild_closest_nsec3_rrsig, wild_closest_nsec3_owner, 
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_RRSIG, min_ttl, headp, pool);
        }
    }

    if((nsec3 != NULL) && (nsec3_rrsig != NULL))
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: nsec3_nodata_error: nsec3_owner: %{dnsname}", nsec3_owner);
#endif
        zdb_query_ex_answer_append_ttl(nsec3, nsec3_owner,                               
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NSEC3, min_ttl, headp, pool);

        zdb_query_ex_answer_appendlist_ttl(nsec3_rrsig, nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_RRSIG, min_ttl, headp, pool);
    }

    if((closest_nsec3 != NULL) && (closest_nsec3_rrsig != NULL))
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: nsec3_nodata_error: closest_nsec3_owner: %{dnsname}", closest_nsec3_owner);
#endif
        zdb_query_ex_answer_append_ttl(closest_nsec3, closest_nsec3_owner,
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NSEC3, min_ttl, headp, pool);
        zdb_query_ex_answer_appendlist_ttl(closest_nsec3_rrsig, closest_nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_RRSIG, min_ttl, headp, pool);
    }
}

/**
 * @brief Appends the wildcard NSEC3 - DATA answer to the section
 * 
 * @param zone the zone
 * @param rr_label the covered label
 * @param name the owner name
 * @param apex_index the index of the apex in the name
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 2 uses
 */
static inline void
zdb_query_ex_append_wild_nsec3_data(const zdb_zone *zone, const zdb_rr_label *rr_label,
                                    const dnsname_vector *name, s32 apex_index,
                                    DECLARE_ZCLASS_PARAMETER
                                    zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zassert(IS_WILD_LABEL(rr_label->name));
    
    nsec3_zone* n3 = zone->nsec.nsec3;

    u8 *nsec3_owner = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u8 *closest_nsec3_owner = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u8 *wild_closest_nsec3_owner = *pool;
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u32 min_ttl;
    zdb_zone_getminttl(zone, &min_ttl);
    
#ifndef NDEBUG
    memset(nsec3_owner, 0xff, MAX_DOMAIN_LENGTH);
    memset(closest_nsec3_owner, 0xff, MAX_DOMAIN_LENGTH);
    memset(wild_closest_nsec3_owner, 0xff, MAX_DOMAIN_LENGTH);
#endif

    zdb_packed_ttlrdata* nsec3;
    zdb_packed_ttlrdata* nsec3_rrsig;
    zdb_packed_ttlrdata* closest_nsec3;
    zdb_packed_ttlrdata* closest_nsec3_rrsig;
    zdb_packed_ttlrdata* wild_closest_nsec3;
    zdb_packed_ttlrdata* wild_closest_nsec3_rrsig;
    
    nsec3_wild_nodata_error(zone, rr_label, name, apex_index, nsec3_owner, &nsec3, &nsec3_rrsig, closest_nsec3_owner, &closest_nsec3, &closest_nsec3_rrsig, wild_closest_nsec3_owner, &wild_closest_nsec3, &wild_closest_nsec3_rrsig);

    if((nsec3 != NULL) && (nsec3_rrsig != NULL))
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: nsec3_nodata_error: nsec3_owner: %{dnsname}", nsec3_owner);
#endif
        zdb_query_ex_answer_append_ttl(nsec3, nsec3_owner, 
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NSEC3, min_ttl, headp, pool);

        zdb_query_ex_answer_appendlist_ttl(nsec3_rrsig, nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_RRSIG, min_ttl, headp, pool);
    }

    if((closest_nsec3 != NULL) && (closest_nsec3_rrsig != NULL))
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: nsec3_nodata_error: closest_nsec3_owner: %{dnsname}", closest_nsec3_owner);
#endif
        zdb_query_ex_answer_append_ttl(closest_nsec3, closest_nsec3_owner, 
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NSEC3, min_ttl, headp, pool);
        zdb_query_ex_answer_appendlist_ttl(closest_nsec3_rrsig, closest_nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_RRSIG, min_ttl, headp, pool);
    }    
}

/**
 * @brief Appends the closest NSEC3 answer to the section (OBSOLETE)
 * 
 * @param zone the zone
 * @param rr_label the covered label
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 0 uses
 */
static inline void
zdb_query_ex_append_closest_nsec3(const zdb_zone *zone, const zdb_rr_label_find_ext_data *rr_label_info,
                                  DECLARE_ZCLASS_PARAMETER
                                  zdb_resourcerecord** headp, u8 * restrict * pool)
{
    zdb_rr_label *give = rr_label_info->closest;

    if((give->nsec.nsec3 == NULL) || (give->nsec.nsec3->self == NULL))
    {
        give = rr_label_info->authority;

        if((give->nsec.nsec3 == NULL) || (give->nsec.nsec3->self == NULL))
        {
            give = zone->apex;
        }
    }

    nsec3_zone* n3 = zone->nsec.nsec3;

    u8* apex_nsec3_owner = *pool;                                    
    *pool += ALIGN16(MAX_DOMAIN_LENGTH);
    
    u32 min_ttl;
    zdb_zone_getminttl(zone, &min_ttl);

    zdb_packed_ttlrdata* apex_nsec3;
    zdb_packed_ttlrdata* apex_nsec3_rrsig;

    nsec3_zone_item_to_zdb_packed_ttlrdata(n3,
                                        give->nsec.nsec3->self,
                                        zone->origin,
                                        apex_nsec3_owner,
                                        min_ttl,
                                        &apex_nsec3,
                                        &apex_nsec3_rrsig);

    zdb_query_ex_answer_append_ttl(apex_nsec3, apex_nsec3_owner, 
                                   PASS_ZCLASS_PARAMETER
                                   TYPE_NSEC3, min_ttl, headp, pool);

    if(apex_nsec3_rrsig != NULL)
    {
        zdb_query_ex_answer_appendlist_ttl(apex_nsec3_rrsig, apex_nsec3_owner, 
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_RRSIG, min_ttl, headp, pool);
    }
}

/**
 * @brief Appends the NSEC3 delegation answer to the section
 * 
 * @param zone the zone
 * @param rr_label the covered label
 * @param name the owner name
 * @param apex_index the index of the apex in the name
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 3 uses
 */
static inline void 
zdb_query_ex_append_nsec3_delegation(const zdb_zone *zone, const zdb_rr_label_find_ext_data *rr_label_info,
                                     const dnsname_vector *name, s32 apex_index,
                                     DECLARE_ZCLASS_PARAMETER
                                     zdb_resourcerecord **headp, u8 * restrict * pool)
{
    zdb_rr_label *authority = rr_label_info->authority;
    
    u32 min_ttl;
    zdb_zone_getminttl(zone, &min_ttl);
    
    if((authority->nsec.nsec3 != NULL) && (authority->nsec.nsec3->self != NULL))
    {
        /* add it */

        u8 *authority_nsec3_owner = *pool;
        *pool += ALIGN16(MAX_DOMAIN_LENGTH);

        nsec3_zone* n3 = zone->nsec.nsec3;
        zdb_packed_ttlrdata *authority_nsec3;
        zdb_packed_ttlrdata *authority_nsec3_rrsig;

        nsec3_zone_item_to_zdb_packed_ttlrdata(n3,
                                            authority->nsec.nsec3->self,
                                            zone->origin,
                                            authority_nsec3_owner,
                                            min_ttl,
                                            &authority_nsec3,
                                            &authority_nsec3_rrsig);

        zdb_query_ex_answer_append_ttl(authority_nsec3, authority_nsec3_owner,
                                       PASS_ZCLASS_PARAMETER
                                       TYPE_NSEC3, min_ttl, headp, pool);

        if(authority_nsec3_rrsig != NULL)
        {
            zdb_query_ex_answer_appendlist_ttl(authority_nsec3_rrsig, authority_nsec3_owner,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_RRSIG, min_ttl, headp, pool);
        }
    }
    else
    {
        /** @todo add closest provable encloser proof */
        
        zdb_query_ex_append_nsec3_nodata(zone, authority, name, apex_index, 0,
                                         PASS_ZCLASS_PARAMETER
                                         headp, pool);
    }
}

#endif

#if ZDB_NSEC_SUPPORT != 0

/**
 * @brief Appends the NSEC records of a label to the section
 * 
 * @param rr_label the covered label
 * @param qname the owner name
 * @param min_ttl the minimum ttl (OBSOLETE)
 * @param zclass (if more than one class is supported in the database)
 * @param headp a pointer to the section list
 * @param pool the memory pool
 * 
 * 2 uses
 */
static inline void
zdb_query_ex_append_nsec_records(const zdb_rr_label *rr_label, const u8 * restrict qname, u32 min_ttl,
                                 DECLARE_ZCLASS_PARAMETER
                                 zdb_resourcerecord **headp, u8 * restrict * pool)
{
    zdb_packed_ttlrdata *rr_label_nsec_record = zdb_record_find(&rr_label->resource_record_set, TYPE_NSEC);
    
    if(rr_label_nsec_record != NULL)
    {
        zdb_query_ex_answer_append(rr_label_nsec_record, qname,
                                   PASS_ZCLASS_PARAMETER
                                   TYPE_NSEC, headp, pool);
        zdb_query_ex_answer_append_type_rrsigs(rr_label, qname, TYPE_NSEC,
                                               PASS_ZCLASS_PARAMETER
                                               rr_label_nsec_record->ttl, headp, pool);
    }
}

#endif

/** @brief Destroys a zdb_resourcerecord* single linked list.
 *
 *  Destroys a zdb_resourcerecord* single linked list created by a zdb_query*
 *
 *  @param[in]  rr the head of the sll.
 * 
 * 3 uses
 */

void
zdb_destroy_resourcerecord_list(zdb_resourcerecord *rr)
{
    while(rr != NULL)
    {
        zdb_resourcerecord *nsec3_rr = rr;

        rr = rr->next;

        if(nsec3_rr->rtype == TYPE_NSEC3)
        {

            zdb_packed_ttlrdata *nsec3_ttl_rdata = nsec3_rr->ttl_rdata;

            do
            {
                zdb_packed_ttlrdata *nsec3_ttl_rdata_prev = nsec3_ttl_rdata;

                nsec3_ttl_rdata = nsec3_ttl_rdata->next;

#ifndef NDEBUG
                memset(nsec3_ttl_rdata_prev, 0xff, sizeof (zdb_packed_ttlrdata));
#endif

                free(nsec3_ttl_rdata_prev);
            }
            while(nsec3_ttl_rdata != NULL);
        }
    }
}

/**
 * @brief Handles what to do when a record has not been found
 * 
 * @param zone the zone
 * @param rr_label_info details about the labels on the path of the query
 * @param qname name of the query
 * @param name name of the query (vector)
 * @param sp index of the label in the name (vector)
 * @param top
 * @param type
 * @param zclass (if more than one class is supported in the database)
 * @param ans_auth_add a pointer to the section list
 * @param pool the memory pool
 * @param additionals_dname_set
 * 
 * 3 uses
 */
static inline ya_result
zdb_query_ex_record_not_found(const zdb_zone *zone,
                              const zdb_rr_label_find_ext_data *rr_label_info,
                              const u8* qname,
                              const dnsname_vector *name,
                              s32 sp_label_index,
                              s32 top,
                              u16 type,
                              DECLARE_ZCLASS_PARAMETER
                              u8 * restrict * pool,
                              bool dnssec,
                              zdb_query_ex_answer *ans_auth_add,
                              dnsname_set *additionals_dname_set)
{
    zdb_rr_label *rr_label = rr_label_info->answer;
    
#if ZDB_NSEC3_SUPPORT != 0
    if(dnssec && ZONE_NSEC3_AVAILABLE(zone))
    {
        zdb_packed_ttlrdata *zone_soa = zdb_record_find(&zone->apex->resource_record_set, TYPE_SOA);
        
        u32 min_ttl;        
        zdb_zone_getminttl(zone, &min_ttl);

        if( ((type == TYPE_DS) && ((rr_label->flags & (ZDB_RR_LABEL_UNDERDELEGATION)) != 0 ))                         ||
            ((type != TYPE_DS) && ((rr_label->flags & (ZDB_RR_LABEL_DELEGATION|ZDB_RR_LABEL_UNDERDELEGATION)) != 0 )) )
        {
            /*
                * Add all the NS and their signature
                */
            zdb_rr_label *authority = rr_label_info->authority;
            zdb_packed_ttlrdata* rr_label_ns = zdb_record_find(&authority->resource_record_set, TYPE_NS);

            if(rr_label_ns != NULL)
            {
                const u8* auth_name = name->labels[rr_label_info->authority_index];

                zdb_query_ex_answer_appendlist(rr_label_ns, auth_name,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_NS, &ans_auth_add->authority, pool);
                zdb_query_ex_answer_append_type_rrsigs(rr_label, auth_name,
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_NS, rr_label_ns->ttl, &ans_auth_add->authority, pool);

                update_additionals_dname_set(rr_label_ns,
                                             PASS_ZCLASS_PARAMETER
                                             TYPE_NS, additionals_dname_set);

                append_additionals_dname_set(zone, 
                                             PASS_ZCLASS_PARAMETER
                                             additionals_dname_set, &ans_auth_add->additional, pool, FALSE);

                zdb_packed_ttlrdata* label_ds = zdb_record_find(&authority->resource_record_set, TYPE_DS);

                if(label_ds != NULL)
                {
                    zdb_query_ex_answer_appendlist(label_ds, auth_name,
                                                   PASS_ZCLASS_PARAMETER
                                                   TYPE_DS, &ans_auth_add->authority, pool);
                    zdb_query_ex_answer_append_type_rrsigs(authority, auth_name, TYPE_DS,
                                                           PASS_ZCLASS_PARAMETER
                                                           label_ds->ttl, &ans_auth_add->authority, pool);
                    
                    /* ans_auth_add->is_delegation = TRUE; later */
                    
                    return FP_BASIC_RECORD_NOTFOUND;
                }
            }
        }
        else
        {
            zdb_query_ex_answer_append_ttl(zone_soa, zone->origin,
                                           PASS_ZCLASS_PARAMETER
                                           TYPE_SOA, min_ttl, &ans_auth_add->authority, pool);
            zdb_query_ex_answer_append_type_rrsigs(zone->apex, zone->origin, TYPE_SOA,
                                                   PASS_ZCLASS_PARAMETER
                                                   min_ttl, &ans_auth_add->authority, pool);
        }

        if(type != 0)
        {
            zdb_query_ex_append_nsec3_nodata(zone, rr_label, name, top, type,
                                             PASS_ZCLASS_PARAMETER
                                             &ans_auth_add->authority, pool);
        }
        else
        {
            /*
                * If there is an NSEC3 RR that matches the delegation name, then that
                * NSEC3 RR MUST be included in the response.  The DS bit in the type
                * bit maps of the NSEC3 RR MUST NOT be set.
                * 
                * If the zone is Opt-Out, then there may not be an NSEC3 RR
                * corresponding to the delegation.  In this case, the closest provable
                * encloser proof MUST be included in the response.  The included NSEC3
                * RR that covers the "next closer" name for the delegation MUST have
                * the Opt-Out flag set to one.  (Note that this will be the case unless
                * something has gone wrong).
                */

            zdb_query_ex_append_nsec3_delegation(zone, rr_label_info, name, top,
                                                 PASS_ZCLASS_PARAMETER
                                                 &ans_auth_add->authority, pool);
        }
#ifndef NDEBUG
        log_debug("zdb_query_ex: FP_NSEC3_RECORD_NOTFOUND (NSEC3)");
#endif
        return FP_NSEC3_RECORD_NOTFOUND;
    }
    else    /* We had the label, not the record, it's not NSEC3 : */
#endif
    {
        /** Got label but no record : show the authority
            *  AA
            */

        /*
            * Gets the NS from, ie: a.nic.eu ... this is wrong.
        zdb_packed_ttlrdata* authority = zdb_record_find(&rr_label->resource_record_set, TYPE_NS);
            */

        if((rr_label_info->authority->flags & ZDB_RR_LABEL_APEX) == 0)
        {
            zdb_packed_ttlrdata* authority;

            if( (
                    ((type == TYPE_DS) && ((rr_label->flags & (ZDB_RR_LABEL_UNDERDELEGATION)) != 0 )) ||
                    ((type != TYPE_DS) && ((rr_label->flags & (ZDB_RR_LABEL_DELEGATION|ZDB_RR_LABEL_UNDERDELEGATION)) != 0 ))
                )
                &&
                (
                    ((authority = zdb_record_find(&rr_label_info->authority->resource_record_set, TYPE_NS)) != NULL)
                ) )
            {
                const u8* auth_name = name->labels[rr_label_info->authority_index];

                zdb_query_ex_answer_appendrndlist(authority, auth_name,
                                                  PASS_ZCLASS_PARAMETER
                                                  TYPE_NS, &ans_auth_add->authority, pool);

                update_additionals_dname_set(authority,
                                             PASS_ZCLASS_PARAMETER
                                             TYPE_NS, additionals_dname_set);
                append_additionals_dname_set(zone,
                                             PASS_ZCLASS_PARAMETER
                                             additionals_dname_set, &ans_auth_add->additional, pool, FALSE);
                
                /* ans_auth_add->is_delegation = TRUE; later */
            }
            else
            {
                /* append the SOA */

                if(!dnssec)
                {
                    zdb_query_ex_answer_append_soa_nttl(zone, &ans_auth_add->authority, pool);
                }
                else
                {
                    zdb_query_ex_answer_append_soa_rrsig_nttl(zone, &ans_auth_add->authority, pool);
                }
            }
        }
        else
        {
            /* append the SOA */

            if(!dnssec)
            {
                zdb_query_ex_answer_append_soa_nttl(zone, &ans_auth_add->authority, pool);
            }
            else
            {
                zdb_query_ex_answer_append_soa_rrsig_nttl(zone, &ans_auth_add->authority, pool);
            }
        }

#if ZDB_NSEC_SUPPORT != 0
        if(dnssec && ZONE_NSEC_AVAILABLE(zone))
        {
            zdb_rr_label* rr_label_authority = rr_label_info->authority;
            zdb_packed_ttlrdata *delegation_signer = zdb_record_find(&rr_label_authority->resource_record_set, TYPE_DS);

            if(delegation_signer != NULL)
            {
                const u8 * authority_qname = zdb_rr_label_info_get_authority_qname(qname, rr_label_info);

                zdb_query_ex_answer_appendlist(delegation_signer , authority_qname,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_DS, &ans_auth_add->authority, pool);                                    
                zdb_query_ex_answer_append_type_rrsigs(rr_label_authority, authority_qname, TYPE_DS,
                                                       PASS_ZCLASS_PARAMETER
                                                       delegation_signer->ttl, &ans_auth_add->authority, pool);
            }
            else
            {
                u8 *wild_name = (u8*)qname;

                if(IS_WILD_LABEL(rr_label->name))
                {
                    wild_name = *pool;
                    *pool += ALIGN16(MAX_DOMAIN_LENGTH + 2);
                    wild_name[0] = 1;
                    wild_name[1] = (u8)'*';
                    dnslabel_vector_to_dnsname(&name->labels[name->size - sp_label_index], sp_label_index, &wild_name[2]);
                }

                zdb_packed_ttlrdata *rr_label_nsec_record = zdb_record_find(&rr_label->resource_record_set, TYPE_NSEC);

                if(rr_label_nsec_record != NULL)
                {
                    zdb_query_ex_answer_append(rr_label_nsec_record, wild_name,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_NSEC, &ans_auth_add->authority, pool);                                
                    zdb_query_ex_answer_append_type_rrsigs(rr_label, wild_name, TYPE_NSEC,
                                                           PASS_ZCLASS_PARAMETER
                                                           rr_label_nsec_record->ttl, &ans_auth_add->authority, pool);
                }

                zdb_query_ex_add_nsec_interval(zone, name, rr_label, &ans_auth_add->authority, pool);
            }
            
        }
#endif
    }
    
    return FP_BASIC_RECORD_NOTFOUND;
}

/**
 * @brief destroys an answer made by zdb_query*
 * 
 * @param ans_auth_add a pointer to the answer structure
 * 
 */

void
zdb_query_ex_answer_destroy(zdb_query_ex_answer* ans_auth_add)
{
    zdb_destroy_resourcerecord_list(ans_auth_add->answer);
    ans_auth_add->answer = NULL;
    zdb_destroy_resourcerecord_list(ans_auth_add->authority);
    ans_auth_add->authority = NULL;
    zdb_destroy_resourcerecord_list(ans_auth_add->additional);
    ans_auth_add->additional = NULL;
}


/**
 * @brief Queries the database given a message
 * 
 * @param db the database
 * @param mesg the message
 * @param ans_auth_add the structure that will contain the sections of the answer
 * @param pool_buffer a big enough buffer used for the memory pool
 * 
 * @return the status of the message (probably useless)
 */

finger_print
zdb_query_ex(const zdb *db, message_data *mesg, zdb_query_ex_answer *ans_auth_add, u8 * restrict pool_buffer)
{
    zassert(ans_auth_add != NULL);

    //const u8 * restrict qname = mesg->qname;
    u8 *qname = mesg->qname;
#if ZDB_RECORDS_MAX_CLASS != 1
    const u16 zclass = mesg->qclass;
#endif
    
    zdb_rr_label_find_ext_data rr_label_info;
    
    u16 type = mesg->qtype;
    const process_flags_t flags = mesg->process_flags;    

    /** Check that we are even allowed to handle that class */

#if ZDB_RECORDS_MAX_CLASS == 1
    if(mesg->qclass != CLASS_IN)
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: FP_CLASS_NOTFOUND");
#endif
        
        return FP_CLASS_NOTFOUND;
    }
#else
    u16 host_zclass = ntohs(zclass); /* no choice */
    if(host_zclass > ZDB_RECORDS_MAX_CLASS)
    {
        return FP_CLASS_NOTFOUND;
    }
#endif

    bool dnssec = (mesg->rcode_ext & RCODE_EXT_DNSSEC) != 0;

    /**
     *  MANDATORY, INITIALISES A LOCAL MEMORY POOL
     *
     *  This is actually a macro found in dnsname_set.h
     */

    dnsname_vector name;
    DEBUG_RESET_dnsname(name);

    dnsname_to_dnsname_vector(qname, &name);

    u8 * restrict * pool = &pool_buffer;

    /*
     * Find closest matching label
     * Should return a stack of zones
     */

    zdb_zone_label_pointer_array zone_label_stack;

#if ZDB_RECORDS_MAX_CLASS == 1
    s32 top = zdb_zone_label_match(db, &name, CLASS_IN, zone_label_stack);
#else
    s32 top = zdb_zone_label_match(db, &name, host_zclass, zone_label_stack);
#endif
    s32 sp = top;

    zdb_packed_ttlrdata* answer;

    /* This flag means that there HAS to be an authority section */

    bool authority_required = flags & PROCESS_FL_AUTHORITY_AUTH;

    /* This flag means the names in the authority must be (internally) resolved if possible */

    bool additionals_required = flags & PROCESS_FL_ADDITIONAL_AUTH;
    
    switch(type)
    {
        case TYPE_DNSKEY:
        {
            authority_required = FALSE;
            additionals_required = FALSE;
            break;
        }
    }
    
#if ZDB_CACHE_ENABLED!=0
    bool found_zones = FALSE;
#endif

    /* Got a stack of zone labels with and without zone cuts */
    /* Search the label on the zone files */

    /* While we have labels along the path */
    
    if(type == TYPE_DS)         // This is the only type that can only be found outside of the zone
    {                           // In order to avoid to hit said zone, I skip the last label.        
        if(name.size == sp - 1) // we have a perfect match (DS for an APEX), try to get outside ...
        {
            s32 parent_sp = sp;
            
            while(--parent_sp >= 0)
            {
                /* Get the "bottom" label (top being ".") */

                zdb_zone_label* zone_label = zone_label_stack[parent_sp];

                /* Is there a zone file at this level ? If yes, search into it. */

                if(zone_label->zone != NULL)
                {
                    // got it.
                    sp = parent_sp;
                    MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;
                    break;
                }
            }
            
            authority_required = FALSE;
        }
    }

    while(sp >= 0)
    {
        /* Get the "bottom" label (top being ".") */

        zdb_zone_label* zone_label = zone_label_stack[sp];

        /* Is there a zone file at this level ? If yes, search into it. */

        if(zone_label->zone != NULL)
        {
#if ZDB_CACHE_ENABLED!=0
            found_zones = TRUE;
#endif
            zdb_zone *zone = zone_label->zone;

            /*
             * We know the zone, and its extension here ...
             */

            {
                /*
                 * Filter handling (ACL)
                 * NOTE: the return code has to be fingerprint-based
                 */

                if(FAIL(zone->query_access_filter(mesg, zone->extension)))
                {
#ifndef NDEBUG
                    log_debug("zdb_query_ex: FP_ACCESS_REJECTED");
#endif
                    return FP_ACCESS_REJECTED;
                }
            }
            
            /**
             * @todo
             * 
             * The ACL have been passed so ... now check that the zone is valid
             */
            
            if(ZDB_ZONE_INVALID(zone))
            {
                /**
                 * @todo the blocks should be reversed and jump if the zone is invalid (help the branch prediction)
                 */
                
#ifndef NDEBUG
                log_debug("zdb_query_ex: FP_ZONE_EXPIRED");
#endif
                
                return FP_INVALID_ZONE;
            }

            //MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;

            dnsname_set additionals_dname_set;
            dnsname_set_init(&additionals_dname_set);

            /*
             * In one query, get the authority and the closest (longest) path to the domain we are looking for.
             */

            zdb_rr_label *rr_label = zdb_rr_label_find_ext(zone->apex, name.labels, name.size - sp, &rr_label_info);

            /* Has a label been found ? */

            if(rr_label != NULL)
            {
                /*
                 * Got the label.  I will not find anything relevant by going
                 * up to another zone file.
                 *
                 * We set the AA bit iff we are not at or under a delegation.
                 *
                 * The ZDB_RR_LABEL_DELEGATION flag means the label is a delegation.
                 * This means that it only contains NS & DNSSEC records + may have sub-labels for glues
                 *
                 * ZDB_RR_LABEL_UNDERDELEGATION means we are below a ZDB_RR_LABEL_DELEGATION label
                 *
                 */
                
                /*
                 * CNAME alias handling
                 */

                if(((rr_label->flags & ZDB_RR_LABEL_HASCNAME) != 0) && (type != TYPE_CNAME) && (type != TYPE_ANY))
                {
                    /*
                    * The label is an alias:
                    * 
                    * Add the CNAME and restart the query from the alias
                    */

                    if(ans_auth_add->depth >= ZDB_CNAME_LOOP_MAX)
                    {
                        log_warn("CNAME depth at %{dnsname} is bigger than allowed %d>=%d", qname, ans_auth_add->depth, ZDB_CNAME_LOOP_MAX);

                        MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;

                        // stop there
                        return FP_CNAME_MAXIMUM_DEPTH;
                    }

                    ans_auth_add->depth++;

                    if((answer = zdb_record_find(&rr_label->resource_record_set, TYPE_CNAME)) != NULL)
                    {
                        /* The RDATA in answer is the fqdn to a label with an A record (list) */
                        /* There can only be one cname for a given owner */
                        /* Append all A/AAAA records associated to the CNAME AFTER the CNAME record */

                        zdb_resourcerecord *rr = ans_auth_add->answer;

                        u32 cname_depth_count = 0; /* I don't want to allocate that globally for now */

                        while(rr != NULL)
                        {
                            if((rr->rtype == TYPE_CNAME) && (ZDB_PACKEDRECORD_PTR_RDATAPTR(rr->ttl_rdata) == ZDB_PACKEDRECORD_PTR_RDATAPTR(answer)))
                            {
                                /* LOOP */

                                log_warn("CNAME loop at %{dnsname}", qname);

                                MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;

                                return FP_CNAME_LOOP;
                            }

                            cname_depth_count++;

                            rr = rr->next;
                        }

                        u8* cname_owner = *pool;

                        *pool += ALIGN16(dnsname_copy(*pool, qname));

                        /* ONE record */
                        zdb_query_ex_answer_append(answer, cname_owner,
                                                    PASS_ZCLASS_PARAMETER
                                                    TYPE_CNAME, &ans_auth_add->answer, pool);

                        if(dnssec)
                        {
                            zdb_query_ex_answer_append_type_rrsigs(rr_label, cname_owner, TYPE_CNAME,
                                                                    PASS_ZCLASS_PARAMETER
                                                                    answer->ttl, &ans_auth_add->answer, pool);                                           
                        }

                        dnsname_copy(mesg->qname, ZDB_PACKEDRECORD_PTR_RDATAPTR(answer));

                        finger_print fp = zdb_query_ex(db, mesg, ans_auth_add, pool_buffer);

                        return fp;
                    }
                    else
                    {
                        /*
                        * We expected a CNAME record but found none.
                        * This is NOT supposed to happen.
                        * 
                        */

                        return FP_CNAME_BROKEN;
                    }
                }

                if((rr_label->flags & (ZDB_RR_LABEL_DELEGATION|ZDB_RR_LABEL_UNDERDELEGATION) ) == 0)
                {
                    MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;
                }
                else
                {
                    /*
                     * we are AT or UNDER a delegation
                     * We can only find (show) NS, DS, RRSIG, NSEC records from the query
                     */                    
                    switch(type)
                    {
                        /* for these ones : give the rrset for the type and clear AA */
                        case TYPE_DS:
                        {
                            if((rr_label->flags & ZDB_RR_LABEL_DELEGATION) != 0)
                            {
                                MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;
                            }
                            else if((rr_label->flags & ZDB_RR_LABEL_UNDERDELEGATION) != 0)
                            {
                                MESSAGE_HIFLAGS(mesg->buffer) &= ~AA_BITS;
                            }
                            authority_required = FALSE;
                            break;
                        }
                        case TYPE_NSEC:
                        {
                            if((rr_label->flags & ZDB_RR_LABEL_UNDERDELEGATION) == 0)
                            {
                                MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;
                            }
                            break;
                        }   
                        /* for these ones : give the rrset for the type */
                        case TYPE_NS:
                            break;
                        /* for this one : present the delegation */
                        case TYPE_ANY:
                            authority_required = FALSE;
                            break;
                        /* for the rest : NSEC ? */
                        default:
                            
                            /* 
                             * do not try to look for it
                             * 
                             * faster: go to label but no record, but let's avoid gotos ...
                             */
                            type = 0;
                            break;
                    }
                }

                /*
                 * First let's handle "simple" cases.  ANY will be handled in another part of the code.
                 */

                if(type != TYPE_ANY)
                {
                    /*
                     * From the label that has been found, get the RRSET for the required type (zdb_packed_ttlrdata*)
                     */

                    if((answer = zdb_record_find(&rr_label->resource_record_set, type)) != NULL)
                    {
                        /* A match has been found */

                        /* NS case */

                        if(type == TYPE_NS)
                        {
                            zdb_resourcerecord **section;

                            /*
                             * If the label is a delegation, the NS have to be added into authority,
                             * else they have to be added into answer.
                             * 
                             */

                            if((rr_label->flags & ZDB_RR_LABEL_DELEGATION) != 0)
                            {
                                section = &ans_auth_add->authority;
                                /* ans_auth_add->is_delegation = TRUE; later */
                            }
                            else
                            {
                                section = &ans_auth_add->answer;
                            }

                            /*
                             * Add the NS records in random order in the right section
                             * 
                             */

                            zdb_query_ex_answer_appendrndlist(answer, qname,
                                                              PASS_ZCLASS_PARAMETER
                                                              type, section, pool);

#if ZDB_DNSSEC_SUPPORT != 0
                            /*
                             * Append all the RRSIG of NS from the label
                             */

                            if(dnssec)
                            {
                                zdb_query_ex_answer_append_type_rrsigs(rr_label, qname, TYPE_NS,
                                                                       PASS_ZCLASS_PARAMETER
                                                                       answer->ttl, section, pool);
                                
                                if((rr_label->flags & ZDB_RR_LABEL_DELEGATION) != 0)
                                {
                                    zdb_packed_ttlrdata* label_ds = zdb_record_find(&rr_label->resource_record_set, TYPE_DS);

                                    if(label_ds != NULL)
                                    {
                                        zdb_query_ex_answer_appendlist(label_ds, qname,
                                                                       PASS_ZCLASS_PARAMETER
                                                                       TYPE_DS, &ans_auth_add->authority, pool);
                                        zdb_query_ex_answer_append_type_rrsigs(rr_label, qname, TYPE_DS,
                                                                               PASS_ZCLASS_PARAMETER
                                                                               label_ds->ttl, &ans_auth_add->authority, pool);
                                    }
                                    else if(ZONE_NSEC3_AVAILABLE(zone))
                                    {
                                        /**
                                        * 
                                        * @todo Referrals to Unsigned Subzones
                                        * 
                                        * If there is an NSEC3 RR that matches the delegation name, then that
                                        * NSEC3 RR MUST be included in the response.  The DS bit in the type
                                        * bit maps of the NSEC3 RR MUST NOT be set.
                                        * 
                                        * If the zone is Opt-Out, then there may not be an NSEC3 RR
                                        * corresponding to the delegation.  In this case, the closest provable
                                        * encloser proof MUST be included in the response.  The included NSEC3
                                        * RR that covers the "next closer" name for the delegation MUST have
                                        * the Opt-Out flag set to one.  (Note that this will be the case unless
                                        * something has gone wrong).
                                        * 
                                        */

                                        zdb_query_ex_append_nsec3_delegation(zone, &rr_label_info, &name, top,
                                                                             PASS_ZCLASS_PARAMETER
                                                                             &ans_auth_add->authority, pool);
                                    }
                                    else if(ZONE_NSEC_AVAILABLE(zone))
                                    {
                                        /*
                                         * Append the NSEC of rr_label and all its signatures
                                         */
                                        
                                        u32 min_ttl = 900;

                                        zdb_zone_getminttl(zone, &min_ttl);
                                        
                                        zdb_query_ex_append_nsec_records(rr_label, qname, min_ttl,
                                                                         PASS_ZCLASS_PARAMETER
                                                                         &ans_auth_add->authority, pool);
                                    }
                                }
                            }
                            
#endif
                            /*
                             * authority is never required since we have it already
                             *
                             */
                            
                            /*
                             * fetch all the additional records for the required type (NS and MX types)
                             * add them to the additional section
                             */

                            if(additionals_required)
                            {
                                update_additionals_dname_set(answer,
                                                             PASS_ZCLASS_PARAMETER
                                                             type, &additionals_dname_set);
                                append_additionals_dname_set(zone,
                                                             PASS_ZCLASS_PARAMETER
                                                             &additionals_dname_set, &ans_auth_add->additional, pool, dnssec);
                            }
                        }
                        else /* general case */
                        {
                            /*
                             * Add the records from the answer in random order to the answer section
                             */

                            zdb_query_ex_answer_appendrndlist(answer, qname,
                                                              PASS_ZCLASS_PARAMETER
                                                              type, &ans_auth_add->answer, pool);

#if ZDB_DNSSEC_SUPPORT != 0
                            /*
                             * Append all the RRSIG of NS from the label
                             */

                            if(dnssec)
                            {
                                zdb_query_ex_answer_append_type_rrsigs(rr_label, qname, type,
                                                                       PASS_ZCLASS_PARAMETER
                                                                       answer->ttl, &ans_auth_add->answer, pool);

                                if(IS_WILD_LABEL(rr_label->name))
                                {
                                    /**
                                     * @todo
                                     * 
                                     * If there is a wildcard match for QNAME and QTYPE, then, in addition
                                     * to the expanded wildcard RRSet returned in the answer section of the
                                     * response, proof that the wildcard match was valid must be returned.
                                     * 
                                     * This proof is accomplished by proving that both QNAME does not exist
                                     * and that the closest encloser of the QNAME and the immediate ancestor
                                     * of the wildcard are the same (i.e., the correct wildcard matched).
                                     * 
                                     * To this end, the NSEC3 RR that covers the "next closer" name of the
                                     * immediate ancestor of the wildcard MUST be returned.
                                     * It is not necessary to return an NSEC3 RR that matches the closest
                                     * encloser, as the existence of this closest encloser is proven by
                                     * the presence of the expanded wildcard in the response.
                                     */

                                    if(ZONE_NSEC3_AVAILABLE(zone))
                                    {
                                        zdb_query_ex_append_wild_nsec3_data(zone, rr_label, &name, top,
                                                                            PASS_ZCLASS_PARAMETER
                                                                            &ans_auth_add->authority, pool);
                                    }
                                    else if(ZONE_NSEC_AVAILABLE(zone))
                                    {
                                        /* add the NSEC of the wildcard and its signature(s) */

                                        zdb_query_ex_add_nsec_interval(zone, &name, NULL, &ans_auth_add->authority, pool);
                                    }
                                }
                            }
#endif
                            /*
                             * if authority required
                             */
                            
                            if(authority_required)
                            {
                                if((type == TYPE_NSEC || type == TYPE_DS) && (rr_label_info.authority != zone->apex))
                                {
                                    rr_label_info.authority = zone->apex;
                                    rr_label_info.authority_index = sp - 1;
                                }
                                
                                zdb_packed_ttlrdata* authority = append_authority(qname,
                                                                                  PASS_ZCLASS_PARAMETER
                                                                                  &rr_label_info, &ans_auth_add->authority, pool, dnssec);

                                if(additionals_required)
                                {
                                    update_additionals_dname_set(authority,
                                                                 PASS_ZCLASS_PARAMETER
                                                                 TYPE_NS, &additionals_dname_set);
                                }
                            }

                            /*
                             * fetch all the additional records for the required type (NS and MX types)
                             * add them to the additional section
                             */

                            if(additionals_required)
                            {
                                update_additionals_dname_set(answer,
                                                             PASS_ZCLASS_PARAMETER
                                                             type, &additionals_dname_set);
                                append_additionals_dname_set(zone,
                                                             PASS_ZCLASS_PARAMETER
                                                             &additionals_dname_set, &ans_auth_add->additional, pool, dnssec);
                            } /* resolve authority */
                        } 

#ifndef NDEBUG
                        log_debug("zdb_query_ex: FP_BASIC_RECORD_FOUND");
#endif
                        return FP_BASIC_RECORD_FOUND;
                    } /* if found the record of the requested type */
                    else
                    {
                        /* label but no record */
                    
                        /**
                        * Got the label, but not the record.
                        * This should branch to NSEC3 if it is supported.
                        */
                    
                        ya_result return_value = zdb_query_ex_record_not_found(zone,
                                                                            &rr_label_info,
                                                                            qname,
                                                                            &name,
                                                                            sp,
                                                                            top,
                                                                            type,
                                                                            PASS_ZCLASS_PARAMETER
                                                                            pool,
                                                                            dnssec,
                                                                            ans_auth_add,
                                                                            &additionals_dname_set);

#ifndef NDEBUG
                        log_debug("zdb_query_ex: FP_BASIC_RECORD_NOTFOUND (done)");
#endif
                        return (finger_print)return_value;
                    }
                }
                else /* We got the label BUT type == TYPE_ANY */
                {
                    if((rr_label->flags & (ZDB_RR_LABEL_DELEGATION|ZDB_RR_LABEL_UNDERDELEGATION) ) == 0)
                    {
                        zdb_packed_ttlrdata *soa = NULL;
                        
                        zdb_packed_ttlrdata *rrsig_list = zdb_record_find(&rr_label->resource_record_set, TYPE_RRSIG);
                        
                        bool answers = FALSE;

                        /* We do iterate on ALL the types of the label */

                        btree_iterator iter;
                        btree_iterator_init(rr_label->resource_record_set, &iter);

                        while(btree_iterator_hasnext(&iter))
                        {
                            btree_node* nodep = btree_iterator_next_node(&iter);

                            u16 type = nodep->hash;
                            
                            answers = TRUE;

                            zdb_packed_ttlrdata* ttlrdata = (zdb_packed_ttlrdata*)nodep->data;

                            /**
                             * @todo: Maybe doing the list once is faster ...
                             *	  And YES maybe, because of the jump and because the list is supposed to
                             *	  be VERY small (like 1-3)
                             */

                            switch(type)
                            {
                                case TYPE_SOA:
                                {
                                    soa = ttlrdata;
                                    continue;
                                }
                                case TYPE_NS:
                                {
                                    /* NO NEED FOR AUTHORITY */
                                    authority_required = FALSE;
                                    /* fallthrough is EXPECTED */
                                }
                                case TYPE_MX:
                                case TYPE_CNAME:
                                {
                                    /* ADD MX "A/AAAA/GLUE" TO ADDITIONAL */

                                    if(additionals_required)
                                    {
                                        update_additionals_dname_set(ttlrdata,
                                                                     PASS_ZCLASS_PARAMETER
                                                                     type, &additionals_dname_set);
                                    }
                                    break;
                                }
                                case TYPE_RRSIG:
                                {
                                    // signatures will be added by type
                                    continue;
                                }
                                default:
                                {
                                    break;
                                }
                            }

                            zdb_query_ex_answer_appendrndlist(ttlrdata, qname,
                                                              PASS_ZCLASS_PARAMETER
                                                              type, &ans_auth_add->answer, pool);
                            
                            if(rrsig_list != NULL)
                            {
                                zdb_query_ex_answer_append_type_rrsigs_from(rrsig_list, qname, type,
                                                                            PASS_ZCLASS_PARAMETER
                                                                            ttlrdata->ttl, &ans_auth_add->answer, pool);
                            }
                        }

                        /* now we can insert the soa, if any has been found, at the head of the list */

                        if(soa != NULL)
                        {
                            zdb_resourcerecord* soa_rr = zdb_query_ex_answer_make(soa, qname,
                                                                                  PASS_ZCLASS_PARAMETER
                                                                                  TYPE_SOA, pool);
                            soa_rr->next = ans_auth_add->answer;
                            ans_auth_add->answer = soa_rr;
                            
                            if(rrsig_list != NULL)
                            {
                                zdb_query_ex_answer_append_type_rrsigs_from(rrsig_list, qname, TYPE_SOA,
                                                                            PASS_ZCLASS_PARAMETER
                                                                            soa_rr->ttl, &ans_auth_add->answer, pool);
                            }
                        }
                        
                        if(answers)
                        {
                            if(authority_required)
                            {   // not at or under a delegation
                                zdb_packed_ttlrdata* authority = append_authority(qname,
                                                                                  PASS_ZCLASS_PARAMETER
                                                                                  &rr_label_info, &ans_auth_add->authority, pool, dnssec);

                                if(additionals_required)
                                {
                                    update_additionals_dname_set(authority,
                                                                 PASS_ZCLASS_PARAMETER
                                                                 TYPE_NS, &additionals_dname_set);
                                }

                            } /* if authority required */

                            if(additionals_required)
                            {
                                append_additionals_dname_set(zone,
                                                             PASS_ZCLASS_PARAMETER
                                                             &additionals_dname_set, &ans_auth_add->additional, pool, dnssec);
                            }
                            
                            if(dnssec && IS_WILD_LABEL(rr_label->name))
                            {
                                /**
                                    * @todo
                                    * 
                                    * If there is a wildcard match for QNAME and QTYPE, then, in addition
                                    * to the expanded wildcard RRSet returned in the answer section of the
                                    * response, proof that the wildcard match was valid must be returned.
                                    * 
                                    * This proof is accomplished by proving that both QNAME does not exist
                                    * and that the closest encloser of the QNAME and the immediate ancestor
                                    * of the wildcard are the same (i.e., the correct wildcard matched).
                                    * 
                                    * To this end, the NSEC3 RR that covers the "next closer" name of the
                                    * immediate ancestor of the wildcard MUST be returned.
                                    * It is not necessary to return an NSEC3 RR that matches the closest
                                    * encloser, as the existence of this closest encloser is proven by
                                    * the presence of the expanded wildcard in the response.
                                    */

                                if(ZONE_NSEC3_AVAILABLE(zone))
                                {
                                    zdb_query_ex_append_wild_nsec3_data(zone, rr_label, &name, top,
                                                                        PASS_ZCLASS_PARAMETER
                                                                        &ans_auth_add->authority, pool);
                                }
                                else if(ZONE_NSEC_AVAILABLE(zone))
                                {
                                    /* add the NSEC of the wildcard and its signature(s) */

                                    zdb_query_ex_add_nsec_interval(zone, &name, NULL, &ans_auth_add->authority, pool);
                                }
                            }
                            
#ifndef NDEBUG
                            log_debug("zdb_query_ex: FP_BASIC_RECORD_FOUND (any)");
#endif
                            return FP_BASIC_RECORD_FOUND;
                        }
                        else
                        {
                            /* no records found ... */
                            
                            return (finger_print)zdb_query_ex_record_not_found(zone,
                                                                                    &rr_label_info,
                                                                                    qname,
                                                                                    &name,
                                                                                    sp,
                                                                                    top,
                                                                                    TYPE_ANY,
                                                                                    PASS_ZCLASS_PARAMETER
                                                                                    pool,
                                                                                    dnssec,
                                                                                    ans_auth_add,
                                                                                    &additionals_dname_set);
                        }
                    }
                    else
                    {   /* ANY, at or under a delegation */
                                                
                        zdb_query_ex_record_not_found(zone,
                              &rr_label_info,
                              qname,
                              &name,
                              sp,
                              top,
                              0,
                              PASS_ZCLASS_PARAMETER
                              pool,
                              dnssec,
                              ans_auth_add,
                              &additionals_dname_set);
                        
                        return FP_BASIC_RECORD_FOUND;
                    }
                }
            }       /* end of if rr_label!=NULL => */
            else    /* rr_label == NULL */
            {
                zdb_rr_label* rr_label_authority = rr_label_info.authority;

                if(rr_label_authority != zone->apex)
                {
                    MESSAGE_HIFLAGS(mesg->buffer) &= ~AA_BITS;
                    
                    zdb_packed_ttlrdata *authority = zdb_record_find(&rr_label_authority->resource_record_set, TYPE_NS);

                    if(authority != NULL)
                    {
                        const u8 * authority_qname = zdb_rr_label_info_get_authority_qname(qname, &rr_label_info);
                        
                        zdb_query_ex_answer_appendrndlist(authority, authority_qname,
                                                          PASS_ZCLASS_PARAMETER
                                                          TYPE_NS, &ans_auth_add->authority, pool);
                        update_additionals_dname_set(authority,
                                                     PASS_ZCLASS_PARAMETER
                                                     TYPE_NS, &additionals_dname_set);
                        append_additionals_dname_set(zone,
                                                     PASS_ZCLASS_PARAMETER
                                                     &additionals_dname_set, &ans_auth_add->additional, pool, FALSE);
                        
                        if(dnssec)
                        {
                            zdb_query_ex_answer_append_type_rrsigs(rr_label_authority, authority_qname, TYPE_NS,
                                                                   PASS_ZCLASS_PARAMETER
                                                                   authority->ttl, &ans_auth_add->authority, pool);
                            
                            zdb_packed_ttlrdata *delegation_signer = zdb_record_find(&rr_label_authority->resource_record_set, TYPE_DS);
                            
                            if(delegation_signer != NULL)
                            {
                                zdb_query_ex_answer_appendlist(delegation_signer , authority_qname,
                                                               PASS_ZCLASS_PARAMETER
                                                               TYPE_DS, &ans_auth_add->authority, pool);                                    
                                zdb_query_ex_answer_append_type_rrsigs(rr_label_authority, authority_qname, TYPE_DS,
                                                                       PASS_ZCLASS_PARAMETER
                                                                       delegation_signer->ttl, &ans_auth_add->authority, pool);
                            }
                            else
                            {
                                if(ZONE_NSEC3_AVAILABLE(zone))
                                {
                                    // add ... ? it looks like the record that covers the path that has been found in the zone
                                    // is used for the digest, then the interval is shown
                                    // add apex NSEC3 (wildcard)

                                    zdb_query_ex_append_nsec3_delegation(zone, &rr_label_info, &name, top,
                                                                         PASS_ZCLASS_PARAMETER
                                                                         &ans_auth_add->authority, pool);
                                }
                                else if(ZONE_NSEC_AVAILABLE(zone))
                                {
                                    /*
                                        * Append the NSEC of rr_label and all its signatures
                                        */

                                    u32 min_ttl = 900;

                                    zdb_zone_getminttl(zone, &min_ttl);

                                    zdb_query_ex_append_nsec_records(rr_label_authority, authority_qname, min_ttl,
                                                                     PASS_ZCLASS_PARAMETER
                                                                     &ans_auth_add->authority, pool);
                                }
                            }
                        }
#ifndef NDEBUG
                        log_debug("zdb_query_ex: FP_BASIC_LABEL_NOTFOUND (done)");
#endif
                        /* ans_auth_add->is_delegation = TRUE; later */
                        
                        return FP_BASIC_LABEL_DELEGATION;
                    }
                }
                else
                {
                    MESSAGE_HIFLAGS(mesg->buffer) |= AA_BITS;
                }
            }

            /* LABEL NOT FOUND: We stop the processing and falltrough NSEC(3) or the basic case. */

            /* Stop looking, skip cache */
            break;

        } /* if(zone!=NULL) */

        sp--;
    } /* while ... */

    /*************************************************
     *                                               *
     * At this point we are not an authority anymore. *
     *                                               *
     *************************************************/

#if ZDB_CACHE_ENABLED!=0
    /* We exhausted the zone files direct matches.
     * We have to fallback on the global (cache) matches.
     * And it's easy because we have the answer already:
     */

    if(!found_zone && (top == name.size))
    {
        /* We found a perfect match label in the global part of the database */

        if((answer = zdb_record_find(&zone_label_stack[top]->global_resource_record_set, type)) != NULL)
        {
            /* *ttlrdata_out for the answer */
            /* How do I find "authority" ? */
            /* From authority, it's easy to find the additionals */

            zdb_query_ex_answer_appendrndlist(answer, qname,
                                              PASS_ZCLASS_PARAMETER
                                              type, &ans_auth_add->answer);
            /*
            ans_auth_add->authority=NULL;
            ans_auth_add->additional=NULL;
             */

            return FP_BASIC_RECORD_FOUND;
        }
    }

#endif

    /*if(authority_required) { */
    /*
     * Get the most relevant label (lowest zone).
     * Try to do NSEC3 or NSEC with it.
     */

    zdb_zone* zone;

    sp = top;
    while(sp >= 0)
    {
        zdb_zone_label* zone_label = zone_label_stack[sp--];

        if((zone = zone_label->zone) != NULL)
        {
            /* if type == DS && zone->origin = qname then the return value is NOERROR instead of NXDOMAIN */
            break;
        }
    }

    if(zone == NULL)
    {
#ifndef NDEBUG
        log_debug("zdb_query_ex: FP_NOZONE_FOUND (2)");
#endif
        
        // ??? zone_pointer_out->apex->flags |= ZDB_RR_LABEL_MASTER_OF;
        
        return FP_NOZONE_FOUND;
    }
    
    if(ZDB_ZONE_INVALID(zone))
    {
        /**
            * @todo the blocks should be reversed and jump if the zone is invalid (help the branch prediction)
            */

#ifndef NDEBUG
        log_debug("zdb_query_ex: FP_ZONE_EXPIRED (2)");
#endif

        return FP_INVALID_ZONE;
    }
    

    /* zone is the most relevant zone */

#if ZDB_NSEC3_SUPPORT != 0

    if(dnssec)
    {
        if(ZONE_NSEC3_AVAILABLE(zone))
        {
            nsec3_zone* n3 = zone->nsec.nsec3;

            u8 *next_closer_owner = *pool;
            *pool += ALIGN16(MAX_DOMAIN_LENGTH);
            zdb_packed_ttlrdata* next_closer;
            zdb_packed_ttlrdata* next_closer_rrsig;

            u8 *closer_encloser_owner = *pool;
            *pool += ALIGN16(MAX_DOMAIN_LENGTH);
            zdb_packed_ttlrdata* closer_encloser;
            zdb_packed_ttlrdata* closer_encloser_rrsig;

            u8 *wild_closer_encloser_owner = *pool;
            *pool += ALIGN16(MAX_DOMAIN_LENGTH);
            zdb_packed_ttlrdata* wild_closer_encloser;
            zdb_packed_ttlrdata* wild_closer_encloser_rrsig;

#ifndef NDEBUG
            log_debug("nsec3_name_error");

            memset(next_closer_owner, 0xff, MAX_DOMAIN_LENGTH);
            memset(closer_encloser_owner, 0xff, MAX_DOMAIN_LENGTH);
            memset(wild_closer_encloser_owner, 0xff, MAX_DOMAIN_LENGTH);
#endif

            nsec3_name_error(zone, &name, top,
                             next_closer_owner, &next_closer, &next_closer_rrsig,
                             closer_encloser_owner, &closer_encloser, &closer_encloser_rrsig,
                             wild_closer_encloser_owner, &wild_closer_encloser, &wild_closer_encloser_rrsig
                             );
     
            int min_ttl = zdb_query_ex_answer_append_soa_rrsig_nttl(zone, &ans_auth_add->authority, pool);

#ifndef NDEBUG
            log_debug("zdb_query_ex: nsec3_name_error: next_closer_owner: %{dnsname}", next_closer_owner);
#endif
            
            if(next_closer != NULL /*&& next_closer_rrsig != NULL*/)
            {
                zdb_query_ex_answer_append_ttl(next_closer, next_closer_owner,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_NSEC3, min_ttl, &ans_auth_add->authority, pool);

                if(next_closer_rrsig != NULL)
                {
                    zdb_query_ex_answer_appendlist_ttl(next_closer_rrsig, next_closer_owner,
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_RRSIG, min_ttl, &ans_auth_add->authority, pool);
                }
            }

            if(closer_encloser != NULL/* && closer_encloser_rrsig != NULL*/)
            {
#ifndef NDEBUG
                log_debug("zdb_query_ex: nsec3_name_error: closer_encloser_owner: %{dnsname}", closer_encloser_owner);
#endif
                zdb_query_ex_answer_append_ttl(closer_encloser, closer_encloser_owner,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_NSEC3, min_ttl, &ans_auth_add->authority, pool);

                if(closer_encloser_rrsig != NULL)
                {
                    zdb_query_ex_answer_appendlist_ttl(closer_encloser_rrsig, closer_encloser_owner,
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_RRSIG, min_ttl, &ans_auth_add->authority, pool);
                }
            }

            if(wild_closer_encloser != NULL)
            {
#ifndef NDEBUG
                log_debug("zdb_query_ex: nsec3_name_error: wild_closer_encloser_owner: %{dnsname}", wild_closer_encloser_owner);
#endif
                zdb_query_ex_answer_append_ttl(wild_closer_encloser, wild_closer_encloser_owner,
                                               PASS_ZCLASS_PARAMETER
                                               TYPE_NSEC3, min_ttl, &ans_auth_add->authority, pool);

                if(wild_closer_encloser_rrsig != NULL)
                {
                    zdb_query_ex_answer_appendlist_ttl(wild_closer_encloser_rrsig, wild_closer_encloser_owner,
                                                       PASS_ZCLASS_PARAMETER
                                                       TYPE_RRSIG, min_ttl, &ans_auth_add->authority, pool);
                }
            }

#ifndef NDEBUG
            log_debug("zdb_query_ex: FP_NSEC3_LABEL_NOTFOUND (done)");
#endif
            
            return FP_NSEC3_LABEL_NOTFOUND;
        }

        else /* Following will be either the NSEC answer or just the SOA added in the authority */
#endif /* ZDB_NSEC3_SUPPORT != 0 */

            /* NSEC, if possible */

#if ZDB_NSEC_SUPPORT != 0 /* TODO */
        if(ZONE_NSEC_AVAILABLE(zone))
        {
            /*
             * Unknown and not in the cache : NSEC
             *
             */

            /*
             * zone label stack
             *
             * #0 : top
             * #1 : com, org, ...
             * #2 : example, ...
             *
             * Which is the inverse of the dnslabel stack
             *
             * dnslabel stack
             *
             * #0 : example
             * #1 : com
             * #2 : NOTHING ("." is not stored)
             *
             *
             */

            /*
             * Get the SOA + NSEC + RRIGs for the zone
             */

            
            zdb_rr_label *apex_label = zone->apex;
            zdb_query_ex_answer_append_soa_rrsig_nttl(zone, &ans_auth_add->authority, pool);
            
            u8 *encloser_nsec_name = *pool;            
            *pool += ALIGN16(MAX_DOMAIN_LENGTH);
            u8 *wild_encloser_nsec_name = *pool;
            *pool += ALIGN16(MAX_DOMAIN_LENGTH);
            zdb_rr_label *encloser_nsec_label;
            zdb_rr_label *wildencloser_nsec_label;
            
            nsec_name_error(zone, &name, rr_label_info.closest_index, encloser_nsec_name, &encloser_nsec_label, wild_encloser_nsec_name, &wildencloser_nsec_label);
            
            if(encloser_nsec_label != NULL)
            {
                zdb_packed_ttlrdata *encloser_nsec_rr = zdb_record_find(&encloser_nsec_label->resource_record_set, TYPE_NSEC);
                
                if(encloser_nsec_rr != NULL)
                {
                    zdb_query_ex_answer_append(encloser_nsec_rr, encloser_nsec_name,
                                               DECLARE_ZCLASS_PARAMETER
                                               TYPE_NSEC, &ans_auth_add->authority, pool);

                    zdb_query_ex_answer_append_type_rrsigs(encloser_nsec_label, encloser_nsec_name, TYPE_NSEC,
                                                           DECLARE_ZCLASS_PARAMETER
                                                           encloser_nsec_rr->ttl, &ans_auth_add->authority, pool);

                    if(wildencloser_nsec_label != encloser_nsec_label)
                    {
                        zdb_packed_ttlrdata *wildencloser_nsec_rr = zdb_record_find(&wildencloser_nsec_label->resource_record_set, TYPE_NSEC);

                        if(wildencloser_nsec_rr != NULL)
                        {
                            zdb_query_ex_answer_append(wildencloser_nsec_rr, wild_encloser_nsec_name,
                                                       DECLARE_ZCLASS_PARAMETER
                                                       TYPE_NSEC, &ans_auth_add->authority, pool);

                            zdb_query_ex_answer_append_type_rrsigs(wildencloser_nsec_label, wild_encloser_nsec_name, TYPE_NSEC,
                                                                   DECLARE_ZCLASS_PARAMETER
                                                                   wildencloser_nsec_rr->ttl, &ans_auth_add->authority, pool);
                        }
                    }
                }
            }            
#ifndef NDEBUG
            log_debug("zdb_query_ex: FP_NSEC_LABEL_NOTFOUND (done)");
#endif            
            return FP_NSEC_LABEL_NOTFOUND;
        }
    }

#endif /* ZDB_NSEC_SUPPORT != 0 */    
    
    zdb_query_ex_answer_append_soa_nttl(zone, &ans_auth_add->authority, pool);
    
#ifndef NDEBUG
    log_debug("zdb_query_ex: FP_BASIC_LABEL_NOTFOUND (done)");
#endif
    
    return FP_BASIC_LABEL_NOTFOUND;

    /* } no authority required*/
}

/** @} */
