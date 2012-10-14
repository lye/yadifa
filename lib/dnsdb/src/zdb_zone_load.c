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
/** @defgroup dnsdbzone Zone related functions
 *  @ingroup dnsdb
 *  @brief Functions used to manipulate a zone
 *
 *  Functions used to manipulate a zone
 *
 * @{
 */

#include <dnscore/format.h>
#include <dnscore/logger.h>
#include <dnscore/dnsname.h>
#include <dnscore/bytearray_output_stream.h>

#include "dnsdb/zdb_zone.h"
#include "dnsdb/zdb_zone_label.h"
#include "dnsdb/zdb_rr_label.h"
#include "dnsdb/zdb_record.h"
#include "dnsdb/zdb_icmtl.h"
#include "dnsdb/zdb_sanitize.h"
#include "dnsdb/zdb_utils.h"

#include "dnsdb/zdb_zone_write.h"

#include "dnsdb/zdb_zone_label_iterator.h"

#if ZDB_DNSSEC_SUPPORT != 0
#include "dnsdb/dnskey.h"
#include "dnsdb/dnssec_keystore.h"
#endif

#if ZDB_NSEC3_SUPPORT != 0
#include "dnsdb/nsec3.h"
#endif

#if ZDB_NSEC_SUPPORT != 0
#include "dnsdb/nsec.h"
#endif

#include "dnsdb/zdb_zone_load.h"

extern logger_handle *g_zone_logger;
#define MODULE_MSG_HANDLE g_zone_logger

void
resource_record_init(resource_record* entry)
{
    /* Initialize "os" stream */
    bytearray_output_stream_init(NULL, 0, &entry->os_rdata);

    entry->next    = NULL;
    entry->ttl     = 0;
    entry->type    = 0;
    entry->class   = 0;

#ifndef NDEBUG
    memset(entry->name,0xff,sizeof(entry->name));
    memset(entry->rdata,0xff,sizeof(entry->rdata));
#endif

    entry->name[0] = 0;
    entry->name[1] = 0;
}

void
resource_record_freecontent(resource_record* entry)
{
    zassert(entry != NULL);

    /* free the record  */
    output_stream_close(&entry->os_rdata);
}

void
resource_record_resetcontent(resource_record* entry)
{
    zassert(entry != NULL);

    /* Resets the RDATA output stream so we can fill it again */
    
    bytearray_output_stream_reset(&entry->os_rdata);
}


/**
 * @brief Load a zone in the database.
 *
 * Load a zone in the database.
 * This is clearly MASTER oriented.
 *
 * @param[in] db a pointer to the database
 * @param[in] zone_data a pointer to an opened zone_reader
 * @param[out] zone_pointer_out will contains a pointer to the loaded zone if the call is successful
 *
 * @return an error code.
 *
 */
ya_result
zdb_zone_load(zdb *db, zone_reader *zone_data, zdb_zone **zone_pointer_out, const char *data_path, const u8 *expected_origin, u16 flags)
{
    u8* rdata;
    size_t rdata_len;
    ya_result return_code;
    resource_record entry;
    bool dynupdate_forbidden;
    bool has_dnskey;
    bool nsec3_keys;
    bool nsec_keys;
    bool has_nsec3;
    bool has_nsec;
    bool has_nsec3param;
    bool has_rrsig;
    u32 has_optout;
    u32 has_optin;
    char origin_ascii[MAX_DOMAIN_TEXT_LENGTH + 1];

    /*    ------------------------------------------------------------    */

#if ZDB_NSEC3_SUPPORT != 0
    nsec3_load_context nsec3_context;
#endif

    /* Take full name of a zone file */
/* to break on .eu
    if(expected_origin[0] == 2)
    {
        puts("");
    }
*/
    resource_record_init(&entry);

    if(FAIL(return_code = zone_reader_read_record(zone_data, &entry)))
    {
        resource_record_freecontent(&entry); /* destroys */

        log_err("zone load: failed on first record");

        return return_code;
    }
    
    if(entry.type != TYPE_SOA)
    {
        /* bad */

        resource_record_freecontent(&entry); /* destroys */

        log_err("zone load: first record expected to be an SOA");

        return ZDB_READER_FIRST_RECORD_NOT_SOA;
    }
    
    if(!(dnsname_locase_verify_charspace(entry.name) && dnsname_equals(entry.name, expected_origin)))
    {
        resource_record_freecontent(&entry); /* destroys */

        log_err("zone load: zone is for domain %{dnsname} but %{dnsname} was expected", entry.name, expected_origin);

        return ZDB_READER_ANOTHER_DOMAIN_WAS_EXPECTED;
    }

    /*    ------------------------------------------------------------    */

    dnsname_vector name;
    DEBUG_RESET_dnsname(name);
    u16 zclass = entry.class;

    dnsname_to_dnsname_vector(entry.name, &name);

    /*    ------------------------------------------------------------    */

    /* A Create a non-existing label */
    /* B insert new zone */
    /* C load file into the new zone */

    /* A */

    zdb_zone_label *zone_label = zdb_zone_label_add(db, &name, zclass);

    if(((flags & ZDB_ZONE_MOUNT_ON_LOAD) != 0) && (zone_label->zone != NULL))
    {
        /* Already loaded */

        log_err("zone load: zone %{dnsnamevector} already loaded ", &name);

        resource_record_freecontent(&entry); /** destroys */

        return ZDB_READER_ALREADY_LOADED;
    }
    
    u32 soa_min_ttl = 0;

    rr_soa_get_minimumttl(bytearray_output_stream_buffer(&entry.os_rdata), bytearray_output_stream_size(&entry.os_rdata), &soa_min_ttl);
    
    dnsname_to_cstr(origin_ascii, entry.name);

    dynupdate_forbidden = FALSE;
    has_dnskey = FALSE;
    has_nsec3 = FALSE;
    has_nsec = FALSE;
    nsec3_keys = FALSE;
    nsec_keys = FALSE;
    has_nsec3param = FALSE;
    has_optout = 0;
    has_optin = 0;

    /* B */

    zdb_zone* zone;

    zone = zdb_zone_create(entry.name, zclass);
    
    if(zone == NULL)
    {
        log_err("zone load: unable to load zone %{dnsname} %{dnsclass}", entry.name, &zclass);
        
        return ZDB_ERROR_NOSUCHCLASS;
    }
    
    zone->min_ttl = soa_min_ttl;

    dnsname_to_dnsname_vector(zone->origin, &name);
    /*rr_entry_freecontent(&entry);*/

    /* C */

#if ZDB_NSEC3_SUPPORT != 0
    nsec3_load_init(&nsec3_context, zone);
#endif

    zone->apex->flags |= ZDB_RR_APEX_LABEL_LOADING;

    zdb_packed_ttlrdata* ttlrdata;

    u32 loop_count;

    for(loop_count = 1;; loop_count++)
    {
        /* Add the entry */
        
        if(dnscore_shuttingdown())
        {
            return_code = STOPPED_BY_APPLICATION_SHUTDOWN;
            break;
        }

        dnsname_vector entry_name;

        DEBUG_RESET_dnsname(entry_name);
        dnsname_to_dnsname_vector(entry.name, &entry_name);

        s32 a_i, b_i;

        if((a_i = name.size) > (b_i = entry_name.size))
        {
            // error

            return_code = ZDB_ERROR_WRONGNAMEFORZONE;

            log_err("zone load: domain name %{dnsnamestack} is too big", &entry_name);

            break;
        }
        
        /* ZONE ENTRY CHECK */

        while(a_i >= 0)
        {
            u8* a = name.labels[a_i--];
            u8* b = entry_name.labels[b_i--];

            if(!dnslabel_equals(a, b))
            {
                log_warn("zone load: bad domain name %{dnsnamestack} for zone %{dnsnamestack}", &entry_name, &name);

                //rr_entry_freecontent(&entry);

                goto zdb_zone_load_loop;
            }
        }

        if(FAIL(return_code))
        {
            break;
        }

        rdata_len = bytearray_output_stream_size(&entry.os_rdata);
        rdata = bytearray_output_stream_buffer(&entry.os_rdata);       
        
#if ZDB_NSEC3_SUPPORT != 0

        /*
         * SPECIAL NSEC3 support !!!
         *
         * If the record is an RRSIG(NSEC3), an NSEC3, or an NSEC3PARAM then
         * it cannot be handled the same way as the others.
         *
         */

        if(entry.type == TYPE_NSEC3PARAM)
        {
            if(FAIL(return_code = nsec3_load_add_nsec3param(&nsec3_context, rdata, rdata_len)))
            {
                break;
            }
            
            ZDB_RECORD_ZALLOC(ttlrdata, /*entry.ttl*/0, rdata_len, rdata);
            zdb_zone_record_add(zone, entry_name.labels, (entry_name.size - name.size) - 1, entry.type, ttlrdata);

            has_nsec3param = TRUE;
        }
        else if(entry.type == TYPE_NSEC3)
        {
            bool rdata_optout = NSEC3_RDATA_IS_OPTOUT(rdata);
            if(rdata_optout)
            {
                has_optout++;
            }
            else
            {
                has_optin++;
            }
            
            if(FAIL(return_code = nsec3_load_add_nsec3(&nsec3_context, entry.name, entry.ttl, rdata, rdata_len)))
            {
                break;
            }
            
            has_nsec3 = TRUE;
        }
        else if(entry.type == TYPE_RRSIG && ((GET_U16_AT(*rdata)) == TYPE_NSEC3)) /** @note : NATIVETYPE */
        {
            if(rdata_len >= 12)
            {
                u32 valid_until = ntohl(GET_U32_AT(rdata[8]));  /* offset the the "valid until" 32 bits field */
                zone->sig_invalid_first = MIN(valid_until, zone->sig_invalid_first);
            }

            if(FAIL(return_code = nsec3_load_add_rrsig(&nsec3_context, entry.name, /*entry.ttl*/soa_min_ttl, rdata, rdata_len)))
            {
                break;
            }
        }
        else
        {
#endif
            /*
             * This is the general case
             * It happen with NSEC3 support if the type is neither NSEC3PARAM, NSEC3 nor RRSIG(NSEC3)
             */
            switch(entry.type)
            {
                case TYPE_DNSKEY:
                {
#if ZDB_DNSSEC_SUPPORT != 0
                    /*
                     * Check if we have access to the private part of the key
                     */

                    u16 tag = dnskey_getkeytag(rdata, rdata_len);
                    u16 flags = ntohs(GET_U16_AT(rdata[0]));
                    u8 algorithm = rdata[3];

                    switch(algorithm)
                    {
                        case DNSKEY_ALGORITHM_DSASHA1_NSEC3:
                        case DNSKEY_ALGORITHM_RSASHA1_NSEC3:
                        {
                            nsec3_keys = TRUE;
                            break;
                        }
                        case DNSKEY_ALGORITHM_DSASHA1:
                        case DNSKEY_ALGORITHM_RSASHA1:
                        {
                            nsec_keys = TRUE;
                            break;
                        }
                        default:
                        {
                            log_info("zone load: unknown key algorithm for K%{dnsname}+%03d+%05hd", zone->origin, algorithm, tag);
                            break;
                        }
                    }

                    dnssec_key *key;
                    
                    /* @TODO use defines */
                    if(ISOK(return_code = dnssec_key_load_private(algorithm, tag, flags, origin_ascii, &key)))
                    {
                        log_info("zone load: loaded private key K%{dnsname}+%03d+%05hd", zone->origin, algorithm, tag);
                        
                        has_dnskey = TRUE;
                    }
                    else
                    {
                        log_warn("zone load: unable to load private key K%{dnsname}+%03d+%05hd: %r", zone->origin, algorithm, tag, return_code);

                        /*
                         * The private key is not available.  Get the public key for signature verifications.
                         */

                        if(ISOK(return_code = dnskey_load_public(rdata, rdata_len, origin_ascii, &key)))
                        {
                            log_info("zone load: loaded public key K%{dnsname}+%03d+%05hd", zone->origin, algorithm, tag);
                            
                            has_dnskey = TRUE;
                        }
                        else
                        {
                            /* the key is wrong */
                            log_warn("zone load: unable to load public key K%{dnsname}+%03d+%05hd: %r", zone->origin, algorithm, tag, return_code);
                        }
                    }
#else
                    /* DNSKEY not supported */
#endif
                    ZDB_RECORD_ZALLOC(ttlrdata, entry.ttl, rdata_len, rdata);
                    zdb_zone_record_add(zone, entry_name.labels, (entry_name.size - name.size) - 1, entry.type, ttlrdata); /* class is implicit */
                    break;
                }
#if ZDB_NSEC_SUPPORT != 0
                case TYPE_NSEC:
                {
                    has_nsec = TRUE;
                    ZDB_RECORD_ZALLOC(ttlrdata, entry.ttl, rdata_len, rdata);
                    zdb_zone_record_add(zone, entry_name.labels, (entry_name.size - name.size) - 1, entry.type, ttlrdata); /* class is implicit */
                    break;
                }
#endif
                case TYPE_RRSIG:
                {
#if ZDB_DNSSEC_SUPPORT == 0
                    if(!has_rrsig)
                    {
                        log_warn("zone load: type %{dnstype} is not supported", &entry.type);
                    }
#else
                    if(rdata_len >= 12)
                    {
                        u32 valid_until = ntohl(GET_U32_AT(rdata[8]));  /* offset the the "valid until" 32 bits field */
                        zone->sig_invalid_first = MIN(valid_until, zone->sig_invalid_first);
                    }
#endif
                    if((GET_U16_AT(*rdata)) == TYPE_NSEC3PARAM)
                    {
                        entry.ttl = 0;
                    }
                    
                    has_rrsig = TRUE;
                    
                    /* falltrough */
                }
                default:
                {
                    ZDB_RECORD_ZALLOC(ttlrdata, entry.ttl, rdata_len, rdata);
                    zdb_zone_record_add(zone, entry_name.labels, (entry_name.size - name.size) - 1, entry.type, ttlrdata); /* class is implicit */
                    break;
                }
#if ZDB_NSEC3_SUPPORT == 0
                case TYPE_NSEC3PARAM:
                {
                    if(!has_nsec3param)
                    {
                        log_warn("zone load: type %{dnstype} is not supported", &entry.type);
                    }
                    has_nsec3param = TRUE;
                    break;
                }
                case TYPE_NSEC3:
                {
                    if(!has_nsec3)
                    {
                        log_warn("zone load: type %{dnstype} is not supported", &entry.type);
                    }
                    has_nsec3 = TRUE;
                    break;
                }
#endif
#if ZDB_NSEC_SUPPORT == 0
                case TYPE_NSEC:
                {
                    if(!has_nsec)
                    {
                        log_warn("zone load: type %{dnstype} is not supported", &entry.type);
                    }
                    has_nsec = TRUE;
                    break;
                }
#endif

            }

#if ZDB_NSEC3_SUPPORT != 0
        }
#endif

zdb_zone_load_loop:

        resource_record_resetcontent(&entry); /* "next" */

        /**
         * Note : Return can be
         *
         * OK:		got a record
         * 1:		end of zone file
         * error code:	failure
         */

        if(OK != (return_code = zone_reader_read_record(zone_data, &entry)))
        {
            if(FAIL(return_code))
            {
                log_err("zone load: reading record #%d of zone %{dnsname}: %r", loop_count, zone->origin, return_code);
            }
            break;
        }

        if(!dnsname_locase_verify_charspace(entry.name))
        {
            /** @todo handle this issue*/
        }

    }

#if ZDB_DEBUG_ZONEFILE_BESTEFFORT==0
zdb_zone_load_exit:
#endif

    resource_record_freecontent(&entry); /* destroys, not "next" */

    zone->apex->flags &= ~ZDB_RR_APEX_LABEL_LOADING;

    if(dynupdate_forbidden)
    {
        log_info("zone load: freezing zone %{dnsname}", zone->origin);
        
        zone->apex->flags |= ZDB_RR_APEX_LABEL_FROZEN;
    }

    if(ISOK(return_code))
    {
        log_info("zone load: sanity check for %{dnsname}", zone->origin);

        if(FAIL(return_code = zdb_sanitize_zone(zone)))
        {
            log_err("zone load: impossible to sanitise %{dnsname}, dropping zone", zone->origin);
        }
        else
        {
            log_info("zone load: sanity check for %{dnsname} done", zone->origin);
        }
    }

#if ZDB_DNSSEC_SUPPORT != 0

    if(ISOK(return_code))
    {
        if(has_nsec3 & !has_nsec3param)
        {
            log_err("zone load: zone %{dnsname} has NSEC3 but no NSEC3PARAM", zone->origin);
            
            return_code = ZDB_READER_NSEC3WITHOUTNSEC3PARAM;
        }
        
        if(has_nsec3param & !has_nsec3)
        {
            log_warn("zone load: zone %{dnsname} has NSEC3PARAM but no NSEC3", zone->origin);
            
            /* force it for generation */
            
            has_nsec3 = true;
        }
        
        if(has_nsec && has_nsec3)
        {
            log_err("zone load: zone %{dnsname} has both NSEC and NSEC3 records !", zone->origin);
            
            return_code = ZDB_READER_MIXED_DNSSEC_VERSIONS;
            
            /**
             * 
             * @todo DROP NSEC (?)
             * 
             */
            
        }
        if(has_nsec3 && !nsec3_keys)
        {
            /* @TODO what do I do ? */
            
            log_err("zone load: zone %{dnsname} is NSEC3 but there are no NSEC3 keys available", zone->origin);
            
        }
        if(has_nsec && !nsec_keys)
        {
            /* @TODO what do I do ? */
            
            log_err("zone load: zone %{dnsname} is NSEC but there are no NSEC keys available", zone->origin);
        }
    }

    if(ISOK(return_code))
    {
        if(has_nsec3)
        {

#if ZDB_NSEC3_SUPPORT != 0
            /**
             * @todo Check if there is both NSEC & NSEC3.  Reject if yes. (LATER On hold until NSEC is back in)
             *       compile NSEC if any
             *   compile NSEC3 if any
             *
             * I'm only doing NSEC3 here.
             */
            
            if((flags & ZDB_ZONE_DNSSEC_MASK) == ZDB_ZONE_NSEC)
            {
                log_warn("zone load: zone %{dnsname} was set to NSEC but is NSEC3", zone->origin);
            }
            
            if(has_optin > 0)
            {
                if(has_optout > 0)
                {
                    log_warn("zone load: zone %{dnsname} has got both OPT-OUT and OPT-IN records (%u and %u)", zone->origin, has_optout, has_optin);
                }
                
                nsec3_context.opt_out = FALSE;
                
                if((flags & ZDB_ZONE_DNSSEC_MASK) == ZDB_ZONE_NSEC3_OPTOUT)
                {
                    log_warn("zone load: zone %{dnsname} was set to OPT-OUT but is OPT-IN", zone->origin);
                }
            }
            else if(has_optout > 0)
            {
                /* has_optin is false and has_optout is true */
                
                if((flags & ZDB_ZONE_DNSSEC_MASK) == ZDB_ZONE_NSEC3)
                {
                    log_warn("zone load: zone %{dnsname} was set to OPT-IN but is OPT-OUT (%u)", zone->origin, has_optout);
                }
            }
            else /* use the configuration */
            {
                nsec3_context.opt_out = ((flags & ZDB_ZONE_DNSSEC_MASK) == ZDB_ZONE_NSEC3_OPTOUT)?TRUE:FALSE;
            }
            
            log_info("zone load: zone %{dnsname} is %s", zone->origin, (nsec3_context.opt_out)?"OPT-OUT":"OPT-IN");
            
            /* If there is something in the NSEC3 context ... */

            if(!nsec3_load_is_context_empty(&nsec3_context))
            {
                /* ... do it. */

                log_debug("zone load: zone %{dnsname}: NSEC3 post-processing.", zone->origin);

                return_code = nsec3_load_compile(&nsec3_context);
                
                if(((flags & ZDB_ZONE_IS_SLAVE) != 0) && (nsec3_context.nsec3_rejected > 0))
                {
                    return_code = DNSSEC_ERROR_NSEC3_INVALIDZONESTATE;
                }
                
                if(ISOK(return_code))
                {
                    log_debug("zone load: zone %{dnsname}: NSEC3 post-processing done", zone->origin);
                }
                else
                {
                    log_debug("zone load: zone %{dnsname}: error %r: NSEC3 post-processing failed", zone->origin, return_code);
                }
            }
            else
            {
                log_debug("zone load: zone %{dnsname}: NSEC3 context is empty", zone->origin);
            }

#ifndef NDEBUG
            nsec3_check(zone);
#endif

#else
            log_err("zone load: zone %{dnsname} has NSEC3* record(s) but the server has been compiled without NSEC support", zone->origin);
#endif
        }
        else if(has_nsec)
        {
            /**
             * @TODO build the nsec chain
             */
            
            if((flags & ZDB_ZONE_DNSSEC_MASK) >= ZDB_ZONE_NSEC3)
            {
                log_warn("zone load: zone %{dnsname} was set to NSEC3 but is NSEC", zone->origin);
            }

#if ZDB_NSEC_SUPPORT != 0

            log_debug("zone load: zone %{dnsname}: NSEC post-processing.", zone->origin);

            return_code = nsec_update_zone(zone);

#else
            log_err("zone load: zone %{dnsname} has NSEC record(s) but the server has been compiled without NSEC support", zone->origin);
#endif
        }
    }
#endif

#if ZDB_NSEC3_SUPPORT != 0
    nsec3_load_destroy(&nsec3_context);
#endif

    if((flags & ZDB_ZONE_MOUNT_ON_LOAD) != 0)
    {
        if(ISOK(return_code))
        {
            log_info("zone load: zone %{dnsname} has been loaded (%d record(s) parsed)", zone->origin, loop_count);

            /**
            * @todo remove the expired placeholder here
            */

            zone_label->zone = zone;

            *zone_pointer_out = zone;
        }
        else
        {
            log_err("zone load: zone %{dnsname}: error %r (%d record(s) parsed)", zone->origin, return_code, loop_count);

            /**
            * @note: Used to call zdb_zone_label_delete(db, &name, zclass);
            * 
            * This is wrong.  This is a direct manipulator (a killer without business logic !)
            * 
            * ex:  database with it.eurid.eu & eu
            *      if you ask to delete eu it will take the eu node and destroy EVERYTHING on it.
            * 
            * Intead use:
            *
            */

            zdb_zone_unload(db, &name, zclass);

            *zone_pointer_out = NULL;
        }
    }
    else
    {
        if(ISOK(return_code))
        {
            log_info("zone load: zone %{dnsname} has been loaded (%d record(s) parsed)", zone->origin, loop_count);
            
            *zone_pointer_out = zone;
        }
        else
        {
            *zone_pointer_out = NULL;
        }
    }

    if(ISOK(return_code) && ((flags & ZDB_ZONE_REPLAY_JOURNAL) != 0))
    {
        /*
         * The zone file has been read.
         * NSEC structures have been created
         *
         * At this point, the incremental journal should be replayed.
         *
         */

#ifndef NDEBUG
        log_debug("zone load: replaying changes from journal");
#endif
        if(FAIL(return_code = zdb_icmtl_replay(zone, data_path, 0, 0, 0)))
        {
            log_err("zone load: journal replay returned %r", return_code);
        }
        else
        {
            if(return_code > 0)
            {
                log_info("zone load: replayed %d changes from journal", return_code);
            }

#ifndef NDEBUG
            log_debug("zone load: post-replay sanity check for %{dnsname}", zone->origin);
#endif

            if(FAIL(return_code = zdb_sanitize_zone(zone)))
            {
                log_err("zone load: impossible to sanitise %{dnsname}, dropping zone", zone->origin);
            }
            else
            {
                log_info("zone load: post-replay sanity check for %{dnsname} done", zone->origin);
            }
        }

        /*
         * End of the incremental replay
         */
    }

#ifndef NDEBUG
    if(ISOK(return_code))
    {
        if(has_nsec3)
        {
            /* Check the AVL collection */

            nsec3_zone *n3 = zone->nsec.nsec3;


            while(n3 != NULL)
            {
                int depth;

                if((depth = nsec3_avl_check(&n3->items)) < 0)
                {
                    puts("oops"); /* debug-only block */
                    exit(-1);
                }

                n3 = n3->next;
            }

            /* Check the correlations between the two databases (zone + zone.nsec3) */
            
            nsec3_check(zone);

            /* Check there are no left alone domains that should have been linked to the nsec3 collections */

            {
                u32 issues_count = 0;
                
                zdb_rr_label *label;
                u8 fqdn[MAX_DOMAIN_LENGTH];
                zdb_zone_label_iterator iter;

                zdb_zone_label_iterator_init(zone, &iter);

                while(zdb_zone_label_iterator_hasnext(&iter))
                {
                    s32 fqdn_len = zdb_zone_label_iterator_nextname(&iter, fqdn);

                    label = zdb_zone_label_iterator_next(&iter);
                    
                    u16 flags = label->flags;

                    u32 last_issues_count = issues_count;

                    if((flags & ZDB_RR_LABEL_UNDERDELEGATION) == 0) /** @todo !zdb_rr_label_is_glue(label) */
                    {
                        /* APEX or NS+DS */

                        if( ((flags & ZDB_RR_LABEL_APEX) != 0) || (((flags & ZDB_RR_LABEL_DELEGATION) != 0) && (zdb_record_find(&label->resource_record_set, TYPE_DS) != NULL) ) )
                        {
                            /* should be linked */
                            if(label->nsec.nsec3 != NULL)
                            {
                                if(label->nsec.nsec3->self == NULL)
                                {
                                    log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: expected self '%{dnsname}'", fqdn);
                                    issues_count++;
                                }
                                if(label->nsec.nsec3->star == NULL)
                                {
                                    log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: expected star '%{dnsname}'", fqdn);
                                    issues_count++;
                                }
                            }
                            else
                            {
                                log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: expected link '%{dnsname}'", fqdn);
                                issues_count++;
                            }
                        }
                        else
                        {
                            if(label->nsec.nsec3 != NULL)
                            {
                                if(label->nsec.nsec3->self != NULL && label->nsec.nsec3->star != NULL)
                                {
                                    log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: not expected! '%{dnsname}'", fqdn);
                                }
                                else if(label->nsec.nsec3->self == NULL && label->nsec.nsec3->star == NULL)
                                {
                                    log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: needs removal '%{dnsname}'", fqdn);
                                }
                                else
                                {
                                    log_debug("database loading HEURISTIC DEBUG NOTE: NSEC3: is unclean    '%{dnsname}'", fqdn);
                                }
                                issues_count++;
                            }
                        }
                    }
                    else
                    {
                        if(label->nsec.nsec3 != NULL)
                        {
                            if(label->nsec.nsec3->self != NULL && label->nsec.nsec3->star != NULL)
                            {
                                log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: not expected! '%{dnsname}'", fqdn);
                            }
                            else if(label->nsec.nsec3->self == NULL && label->nsec.nsec3->star == NULL)
                            {
                                log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3: needs removal '%{dnsname}'", fqdn);
                            }
                            else
                            {
                                log_debug("database loading HEURISTIC DEBUG NOTE: NSEC3: is unclean    '%{dnsname}'", fqdn);
                            }
                            issues_count++;
                        }
                    }

                    if(last_issues_count != issues_count)
                    {
                        nsec3_zone *n3 = zone->nsec.nsec3;

                        while(n3 != NULL)
                        {
                            u8 digest[MAX_DIGEST_LENGTH];

                            nsec3_compute_digest_from_fqdn(n3, fqdn, digest);

                            log_debug("zone load: HEURISTIC DEBUG NOTE: NSEC3:        check %{digest32h}", digest);

                            n3 = n3->next;
                        }
                    }
                }
/*
                if(issues_count > 0)
                {
                    char file_name[1024];

                    snformat(file_name, sizeof(file_name), "/tmp/%{dnsname}-zone.txt", zone->origin);
                    zdb_zone_write_text_file(zone, file_name, FALSE);
                }
*/
            }
        }
    }
#endif

    // zdb_zone_write_text_file(zone, "/tmp/ars.txt", FALSE);

    return return_code;
}

/**
 * @brief Load the zone SOA.
 *
 * Load the zone SOA record
 * This is meant mainly for the slave that could choose between, ie: zone file or axfr zone file
 * The SOA MUST BE the first record
 *
 * @param[in] db a pointer to the database
 * @param[in] zone_data a pointer to an opened zone_reader at its start
 * @param[out] zone_pointer_out will contains a pointer to the loaded zone if the call is successful
 *
 * @return an error code.
 *
 */
ya_result
zdb_zone_get_soa(zone_reader *zone_data, u16 *rdata_size, u8 *rdata)
{
    ya_result return_value;
    resource_record entry;
    
    resource_record_init(&entry);

    if(ISOK(return_value = zone_reader_read_record(zone_data, &entry)))
    {
        if(entry.type == TYPE_SOA)
        {
            s32 soa_rdata_len = bytearray_output_stream_size(&entry.os_rdata);
            u8 *soa_rdata = bytearray_output_stream_buffer(&entry.os_rdata);
            
            if(soa_rdata_len < MAX_SOA_RDATA_LENGTH)
            {
                memcpy(rdata, soa_rdata, soa_rdata_len);
                *rdata_size = soa_rdata_len;
            }
            else
            {
                return_value = ERROR;
            }
        }
        else
        {
            return_value = ERROR;
        }
    }
    
    return return_value;
}


/**
 * @brief Load the zone serial.
 *
 * Load the zone serial.
 * This is meant mainly for the slave that could choose between, ie: zone file or axfr zone file
 *
 * @param[in] db a pointer to the database
 * @param[in] zone_data a pointer to an opened zone_reader at its start
 * @param[out] zone_pointer_out will contains a pointer to the loaded zone if the call is successful
 *
 * @return an error code.
 *
 */
ya_result
zdb_zone_get_serial(zdb *db, zone_reader *zone_data, const char *data_path, u32 *serial, bool withjournal)
{
    ya_result return_value;
    resource_record entry;
    
    resource_record_init(&entry);

    if(ISOK(return_value = zone_reader_read_record(zone_data, &entry)))
    {
        if(entry.type == TYPE_SOA)
        {
            s32 rdata_len = bytearray_output_stream_size(&entry.os_rdata);
            u8 *rdata = bytearray_output_stream_buffer(&entry.os_rdata);

            for(int i = 2; i> 0; i--)
            {
                for(;;)
                {
                    u8 l = *rdata;

                    rdata_len--;
                    rdata++;

                    if(l == 0)
                    {
                        break;
                    }

                    if(l > rdata_len)
                    {
                        break;
                    }

                    rdata += l;
                    rdata_len -= l;
                }
            }

            if(rdata_len == 20)
            {
                *serial = ntohl(GET_U32_AT(rdata[0]));
                
                /*
                 * we got a serial
                 * 
                 * Now, maybe, we want to know up to when it will replay
                 */
                
                if(withjournal)
                {

#ifndef NDEBUG
                    log_debug("zone load: getting last serial for zone using journal");
#endif
                
                    if(ISOK(return_value = zdb_icmtl_get_last_serial_from(*serial, entry.name, data_path, serial)))
                    {
#ifndef NDEBUG
                        log_debug("zone load: got serial");
#endif
                    }
                    else
                    {
                        log_err("zone load: journal seek: %r", return_value);
                    }
                }
            }
            else
            {
                return_value = ERROR;
            }
        }
        else
        {
            return_value = ERROR;
        }
    }
    
    return return_value;
}


/**
  @}
 */
