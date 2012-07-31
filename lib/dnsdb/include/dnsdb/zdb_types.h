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
/** @defgroup types The types used in the database
 *  @ingroup dnsdb
 *  @brief The types used in the database
 *
 * The types used in the database
 *
 * @{
 */
#ifndef _ZDB_TYPES_H
#define	_ZDB_TYPES_H

#include <dnsdb/zdb_config.h>
#include <dnsdb/zdb_alloc.h>
#include <dnscore/dnsname.h>
#include <dnsdb/dictionary.h>
#include <dnscore/rfc.h>
#include <dnscore/message.h>
#include <dnscore/mutex.h>
#include <dnscore/alarm.h>

#ifdef	__cplusplus
extern "C"
{
#endif

#define ROOT_LABEL                  ((u8*)"")

/* zdb_ttlrdata
 *
 * This record is allocated in:
 *
 * zdb_zone_load
 * zdb_add_global
 *
 *
 */

typedef struct zdb_packed_ttlrdata zdb_packed_ttlrdata;


struct zdb_packed_ttlrdata
{ /* DO NOT CHANGE THE ORDER OF THE FIELDS !!! */
    zdb_packed_ttlrdata* next; /*  4  8 */
    u32 ttl; /*  4  4 */
    u16 rdata_size; /*  2  2 */
    u8 rdata_start[1];
};

#define ZDB_RDATABUF_TAG    0x4655424154414452

#if ZDB_USES_ZALLOC==0

#define ZDB_MEMORY_ZALLOC(cast_,data_,size_)                            \
    {                                                                   \
        MALLOC_OR_DIE(cast_,data_,size_, GENERIC_TAG); /* ZALLOC IMPOSSIBLE */    \
    }

#define ZDB_RECORD_TAG              0x4443455242445a    /** "ZDBRECD" */

#define ZDB_RECORD_ZALLOC(record,ttl_,len_,rdata_)                   \
    {                                                                \
        MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record),sizeof(zdb_packed_ttlrdata)-1+len_,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        (record)->ttl=ttl_;                                          \
        (record)->rdata_size=len_;                                   \
        MEMCOPY(&(record)->rdata_start[0],rdata_,len_);               \
    }

#define ZDB_RECORD_SIZE(record_) (sizeof(zdb_packed_ttlrdata)-1+(record_)->rdata_size)

#define ZDB_RECORD_ZALLOC_EMPTY(record,ttl_,len_)                    \
    {                                                                \
        MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record),sizeof(zdb_packed_ttlrdata)-1+len_,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        (record)->ttl=ttl_;                                          \
        (record)->rdata_size=len_;                                   \
    }

#define ZDB_RECORD_CLONE(record_s_,record_d_)                           \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+(record_s_)->rdata_size; \
        MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record_d_),size,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        record_d_->ttl=record_s_->ttl;                                  \
        record_d_->rdata_size=record_s_->rdata_size;                    \
        MEMCOPY(&(record_d_)->rdata_start[0],&(record_s_)->rdata_start[0],record_s_->rdata_size); \
    }

#define ZDB_RECORD_ZFREE(record) free(record)

#define ZDB_RECORD_SAFE_ZFREE(record) free(record)

#else

/* Do not forget that ZALLOC can only be used where the size of the record
 * can be found from the allocated memory.
 */

#define ZDB_MEMORY_ZALLOC(cast_,data_,size_)                            \
    {                                                                   \
        if(size_<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                        \
        {                                                               \
            (data_)=(cast_)zdb_malloc(((size_)-1)>>3);                  \
        }                                                               \
        else                                                            \
        {                                                               \
            MALLOC_OR_DIE(cast_,data_,size_, GENERIC_TAG); /* ZALLOC IMPOSSIBLE */ \
        }                                                               \
    }

#define ZDB_RECORD_TAG              0x4443455242445a    /** "ZDBRECD" */

#define ZDB_RECORD_SIZE(record_) (sizeof(zdb_packed_ttlrdata)-1+(record_)->rdata_size)

#define ZDB_RECORD_ZALLOC(record_,ttl_,len_,rdata_)                     \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+len_;                    \
        if(size<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                         \
        {                                                               \
            record_=(zdb_packed_ttlrdata*)zdb_malloc((size-1)>>3);   \
        }                                                               \
        else                                                            \
        {                                                               \
            MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record_),sizeof(zdb_packed_ttlrdata)-1+len_,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        }                                                               \
                                                                        \
        (record_)->ttl=ttl_;                                            \
        (record_)->rdata_size=len_;                                     \
        MEMCOPY(&(record_)->rdata_start[0],rdata_,len_);                 \
    }

#define ZDB_RECORD_ZALLOC_EMPTY(record_,ttl_,len_)                      \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+len_;                    \
        if(size<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                         \
        {                                                               \
            record_=(zdb_packed_ttlrdata*)zdb_malloc((size-1)>>3);   \
        }                                                               \
        else                                                            \
        {                                                               \
            MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record_),sizeof(zdb_packed_ttlrdata)-1+len_,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        }                                                               \
                                                                        \
        (record_)->ttl=ttl_;                                            \
        (record_)->rdata_size=len_;                                     \
    }

#define ZDB_RECORD_CLONE(record_s_,record_d_)                           \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+(record_s_)->rdata_size; \
        if(size<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                         \
        {                                                               \
            record_d_=(zdb_packed_ttlrdata*)zdb_malloc((size-1)>>3); \
        }                                                               \
        else                                                            \
        {                                                               \
            MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record_d_),size,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
        }                                                               \
        record_d_->ttl=record_s_->ttl;                                  \
        record_d_->rdata_size=record_s_->rdata_size;                    \
        MEMCOPY(&(record_d_)->rdata_start[0],&(record_s_)->rdata_start[0],record_s_->rdata_size); \
    }

/* DOES NOT CHECKS FOR NULL */
#define ZDB_RECORD_ZFREE(record_)                                       \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+(record_)->rdata_size;   \
        if(size<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                         \
        {                                                               \
            zdb_mfree(record_,(size-1)>>3);                             \
        }                                                               \
        else                                                            \
        {                                                               \
            free(record_);                                              \
        }                                                               \
    }

/* DOES CHECKS FOR NULL */
#define ZDB_RECORD_SAFE_ZFREE(record_)                                  \
    if(record_ != NULL)                                                 \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+(record_)->rdata_size;   \
        if(size<=ZDB_ALLOC_PG_PAGEABLE_MAXSIZE)                         \
        {                                                               \
            zdb_mfree(record_,(size-1)>>3);                             \
        }                                                               \
        else                                                            \
        {                                                               \
            free(record_);                                              \
        }                                                               \
    }



#endif

#define ZDB_RECORD_MALLOC_EMPTY(record_,ttl_,len_)                      \
    {                                                                   \
        u32 size=sizeof(zdb_packed_ttlrdata)-1+len_;                    \
	MALLOC_OR_DIE(zdb_packed_ttlrdata*,(record_),sizeof(zdb_packed_ttlrdata)-1+len_,ZDB_RECORD_TAG); /* ZALLOC IMPOSSIBLE */ \
                                                                        \
        (record_)->ttl=ttl_;                                            \
        (record_)->rdata_size=len_;                                     \
    }

/*
 * These macros existed when 2 different ways for storing the record were
 * available at compile time.
 *
 * The zdb_packed_ttlrdata having proved to be the best (by far),
 * the other one has been removed.
 *
 */

#define ZDB_PACKEDRECORD_PTR_RDATAPTR(record_)  (&(record_)->rdata_start[0])
#define ZDB_PACKEDRECORD_PTR_RDATASIZE(record_) ((record_)->rdata_size)

#define ZDB_RECORD_PTR_RDATASIZE(record_) ((record_)->rdata_size)
#define ZDB_RECORD_PTR_RDATAPTR(record_) ((record_)->rdata_pointer)

typedef struct zdb_ttlrdata zdb_ttlrdata;


struct zdb_ttlrdata
{
    zdb_ttlrdata* next; /*  4  8 */
    u32 ttl; /*  4  4 */
    u16 rdata_size; /*  2  2 */
    u16 padding_reserved;
    void* rdata_pointer; /*  4  8 */
};

#define TTLRDATA_INLINESIZE         THIS_SHOULD_NOT_BE_USED_IN_PACKED_MODE

#define ZDB_RECORD_TTLRDATA_SET(record,ttl_,len_,rdata_)            \
    {                                                               \
        (record).ttl=ttl_;                                          \
        (record).rdata_size=len_;                                   \
        (record).rdata_pointer=rdata_;                                  \
    }

typedef btree zdb_rr_collection;


#define ZDB_RESOURCERECORD_TAG 0x444345524c4c5546   /** "FULLRECD" */

typedef struct zdb_resourcerecord zdb_resourcerecord;


/* zdb_zone */

typedef struct zdb_zone zdb_zone;


#define LABEL_HAS_RECORDS(label_) ((label_)->resource_record_set != NULL)

struct zdb_resourcerecord
{
    zdb_resourcerecord* next; /*  4  8 */
    zdb_packed_ttlrdata* ttl_rdata; /*  4  8 */
    const u8* name; /*  4  8 */
    u16 zclass; /*  2  2 */
    u16 rtype; /*  2  2 */
    u32 ttl;
}; /* 16 28 => 16 32 */

/* zdb_rr_label */

typedef dictionary zdb_rr_label_set;

typedef struct zdb_rr_label zdb_rr_label;

#if ZDB_DNSSEC_SUPPORT != 0

#if ZDB_NSEC_SUPPORT != 0

typedef struct nsec_label_extension nsec_label;
typedef struct nsec_label_extension nsec_label_extension;
typedef struct nsec_node nsec_zone;


#endif

#if ZDB_NSEC3_SUPPORT != 0

typedef struct nsec3_zone nsec3_zone;


#endif

typedef union nsec_zone_union nsec_zone_union;

union nsec_zone_union
{
#if ZDB_NSEC_SUPPORT != 0
    /*
     * A pointer to an array of nsec_label (nsec_label buffer[])
     * The size is the same as the dictionnary
     */
    nsec_zone* nsec;
#endif
    
#if ZDB_NSEC3_SUPPORT != 0
    nsec3_zone* nsec3;
#endif
};

typedef union nsec_label_union nsec_label_union;

struct nsec_label_extension
{
    struct nsec_node* node;
};

union nsec_label_union
{
    /* NSEC */
#if ZDB_NSEC_SUPPORT != 0
    nsec_label_extension nsec;
#endif

    /* NSEC3 */
#if ZDB_NSEC3_SUPPORT != 0
    struct nsec3_label_extension* nsec3;
#endif

    /* Placeholder */
    void* dnssec;
};

#endif

/*
 * RR_LABEL flags
 */


/*
 * For the apex, marks a label as being the apex
 *
 */

#define ZDB_RR_LABEL_APEX           0x0080

/*
 * For any label, marks it as having been updated (for DNSSEC post processing in a dynamic update)
 */
#define ZDB_RR_LABEL_UPDATING       0x0040

/*
 * For any label but the apex : marks it as being a delegation (contains an NS record)
 */

#define ZDB_RR_LABEL_DELEGATION     0x0020

/*
 * Explicitly mark a label as owner of a (single) CNAME
 */

#define ZDB_RR_LABEL_HASCNAME       0x0100

/*
 * Explicitly mark a label as owner of a something that is not a CNAME nor RRSIG nor NSEC
 */

#define ZDB_RR_LABEL_DROPCNAME      0x0200

/*
 * For any label, means there is a delegation (somewhere) above
 */

#define ZDB_RR_LABEL_UNDERDELEGATION 0x0400

/*
 * When a zone has expired, the apex is marked with this flag
 */

#define ZDB_RR_LABEL_INVALID_ZONE   0x0800

/*
 * For any label : marks that one owns a '*' label
 */
#define ZDB_RR_LABEL_GOT_WILD       0x0010

#if ZDB_DNSSEC_SUPPORT != 0
/*
 * This flag means that the label has a valid NSEC structure
 *
 * IT IS NOT VALID TO CHECK THIS TO SEE IF A ZONE IS NSEC
 */
#define ZDB_RR_LABEL_NSEC           0x0001

/*
 * This flag means that the label has a valid NSEC3 structure
 *
 * IT IS NOT VALID TO CHECK THIS TO SEE IF A ZONE IS NSEC3
 */
#define ZDB_RR_LABEL_NSEC3          0x0002

#define ZDB_RR_LABEL_NSEC3_OPTOUT   0x1000

#define ZDB_RR_LABEL_DNSSEC_EDIT    0x0004

#endif

/*
 * Forbid updates to the apex
 */
#define ZDB_RR_APEX_LABEL_FROZEN    0x0040
/**
 * For the apex, marks the zone as being loaded
 * (no NSEC3/RRSIG must be computed while the flag is on)
 */
#define ZDB_RR_APEX_LABEL_LOADING   0x0040

/*
 * This lock is set while a threaded read or a writer wants to edit the zone.
 * Beside the owner, no threaded operation no write can occur while it is on.
 */
#define ZDB_RR_APEX_LABEL_LOCKED    0x0008

#define ZDB_ZONE_FLAGS_LOCKED       0x0001

#define ZDB_ZONE_VALID(__z__) ((((__z__)->apex->flags)&ZDB_RR_LABEL_INVALID_ZONE) == 0)
#define ZDB_ZONE_INVALID(__z__) ((((__z__)->apex->flags)&ZDB_RR_LABEL_INVALID_ZONE) != 0)

#define ZDB_LABEL_UNDERDELEGATION(__l__) ((((__l__)->flags)&ZDB_RR_LABEL_UNDERDELEGATION)!=0)
#define ZDB_LABEL_ATDELEGATION(__l__) ((((__l__)->flags)&ZDB_RR_LABEL_DELEGATION)!=0)

#define ZDB_LABEL_ISAPEX(__l__) ((((__l__)->flags)&ZDB_RR_LABEL_APEX)!=0)

struct zdb_rr_label
{
    zdb_rr_label* next; /* dictionnary_node* next */ /*  4  8 */
    zdb_rr_label_set sub; /* dictionnary of N children labels */ /* 16 24 */

    zdb_rr_collection resource_record_set; /* resource records for the label (a btree)*/ /*  4  4 */

#if ZDB_DNSSEC_SUPPORT != 0
    nsec_label_union nsec;
#endif

    u16 flags;	/* NSEC, NSEC3, and 6 for future usage ... */

    u8 name[1]; /* label */ /*  4  8 */
    /* No zone ptr */
}; /* 28 44 => 32 48 */

#define ZDB_ZONE_MUTEX_NOBODY           0
#define ZDB_ZONE_MUTEX_SIMPLEREADER     0x01 /* non-conflicting */
#define ZDB_ZONE_MUTEX_RRSIG_UPDATER    0x02 /* non-conflicting as far as the global task has not been launched more than ONCE */
#define ZDB_ZONE_MUTEX_NSEC3_UPDATER    0x02 /* non-conflicting as far as ... */
#define ZDB_ZONE_MUTEX_XFR              0x84 /* conflicting, can by nature be launched more than once in parallel.  new ones have to be discarded */
#define ZDB_ZONE_MUTEX_REFRESH          0x85 /* conflicting, can never be launched more than once.  new ones have to be discarded */
#define ZDB_ZONE_MUTEX_DYNUPDATE        0x86 /* conflicting */
#define ZDB_ZONE_MUTEX_UNFREEZE         0x87 /* conflicting, needs to be sure nobody else (ie: the freeze) is acting at the same time */
#define ZDB_ZONE_MUTEX_DESTROY          0xFF /* conflicting, can never be launched more than once.  The zone will be destroyed before unlock. */

typedef ya_result zdb_zone_access_filter(message_data* /*mesg*/, void* /*zone_extension*/);

#define ALARM_KEY_ZONE_SIGNATURE_UPDATE 1
#define ALARM_KEY_ZONE_AXFR_QUERY       2
#define ALARM_KEY_ZONE_REFRESH          3

struct zdb_zone
{
    u8 *origin; /* dnsname, origin */
    zdb_rr_label *apex; /* pointer to the zone cut, 1 name for : SOA, NS, ... */
    
#if ZDB_DNSSEC_SUPPORT != 0
    nsec_zone_union nsec;
#endif

    zdb_zone_access_filter* query_access_filter;
    void *extension;	/**
			 * This pointer is meant to be used by the server so it can associate data with the zone
			 * without having to do the match on its side too.
			 *
			 */
    
    u32 min_ttl;        /* a copy of the min-ttl from the SOA */

#if ZDB_DNSSEC_SUPPORT != 0
    
    u32 sig_validity_regeneration_seconds;
    u32 sig_validity_interval_seconds;
    u32 sig_validity_jitter_seconds;

    /**
     * The earliest epoch at which a signature will be invalidated.
     * This does not account for missing signatures.
     * 0 means both "unknown" and "now"
     */
    
    u32 sig_invalid_first;
#endif
    
    alarm_t alarm_handle;
    u8  mutex_owner;
    u8  mutex_count;

#if ZDB_RECORDS_MAX_CLASS != 1
    u16 zclass;
#endif
    
    mutex_t mutex;

    dnsname_vector origin_vector;

}; /* 18 34 => 20 40 */

/* zdb_zone_label */

typedef dictionary zdb_zone_label_set;

typedef struct zdb_zone_label zdb_zone_label;


struct zdb_zone_label
{
    zdb_zone_label* next;/* used to link labels with the same hash into a SLL */
    zdb_zone_label_set sub; /* labels of the sub-level                           */
    u8* name;               /* label name                                        */
#if ZDB_CACHE_ENABLED!=0
    /* global resource record (used by the cache) */
    zdb_rr_collection global_resource_record_set;
#endif
    zdb_zone* zone; /* zone cut starting at this level                   */
}; /* 32 56 */

typedef zdb_zone_label* zdb_zone_label_pointer_array[DNSNAME_MAX_SECTIONS];

/* zdb */

typedef struct zdb zdb;


struct zdb
{
    zdb_zone_label* root[ZDB_RECORDS_MAX_CLASS];
    alarm_t alarm_handle;
    /* u32 items; */
};

typedef zdb_ttlrdata** zdb_ttlrdata_pointer_array;

/*
 *
 */

typedef struct zdb_query_ex_answer zdb_query_ex_answer;

struct zdb_query_ex_answer
{
    zdb_resourcerecord *answer;
    zdb_resourcerecord *authority;
    zdb_resourcerecord *additional;
    u8 depth;       // CNAME
    /* bool is_delegation; later */
};

/**
 * Iterator through the (rr) labels in a zone
 */

typedef struct zdb_zone_label_iterator zdb_zone_label_iterator;

struct zdb_zone_label_iterator
{
    dnslabel_stack dnslabels;
    dictionary_iterator stack[DNSNAME_MAX_SECTIONS];
    s32 top;
    zdb_rr_label* current_label;
    const zdb_zone* zone;
    s32 current_top;    /* "top" of the label pointer by current_label  */
    s32 prev_top;       /* "top" of the label returned with "_next"     */
};

#ifdef	__cplusplus
}
#endif

#endif	/* _ZDB_TYPES_H */

/** @} */
