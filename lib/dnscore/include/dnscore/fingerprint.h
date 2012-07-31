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
/** @defgroup dnscore
 *  @ingroup dnscore
 *  @brief fingerprints, mapping between answer code and dns code
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#ifndef _FINGERPRINT_H
#define	_FINGERPRINT_H

#include <dnscore/rfc.h>

#ifdef	__cplusplus
extern "C"
{
#endif

/* This makes the finger print of the name server */
enum finger_print
{
    /*
     * Obsolete
     */

    FP_DATABASE_ERR             = RCODE_SERVFAIL,       /* NOT USED */
    FP_ANCOUNT_NOT_0            = RCODE_FORMERR,        /* NOT USED */
    FP_TC_BIT_SET               = RCODE_FORMERR,        /* NOT USED */
    FP_Z_BITS_SET               = RCODE_FORMERR,        /* NOT USED */
    FP_RCODE_BITS_SET           = RCODE_FORMERR,        /* NOT USED */
    FP_ZOCOUNT_NOT_1            = RCODE_FORMERR,        /* NOT USED */
    FP_CH_REFUSED               = RCODE_FORMERR,        /* NOT USED */    
    FP_NOT_SUPP_CLASS           = RCODE_NOTIMP,         /* NOT USED, same remark as for FP_NOT_SUPP_TYPE */

    FP_NSCOUNT_NOT_0            = RCODE_FORMERR,        /* Message processing.  Not used anymore (conflicts with updates) */

    /*
     * Message processing
     */

    FP_PACKET_DROPPED           = RCODE_FORMERR,        /* */
    
    FP_MESG_OK                  = RCODE_NOERROR,        /* The message processing didn't found anything wrong with
                                                         * the query.
                                                         */

    FP_QDCOUNT_BIG_1            = RCODE_FORMERR,        /* The message processing rejected the query because #QD>1
                                                         * which is not supported by yadifa
                                                         */
    
    FP_QDCOUNT_IS_0             = RCODE_FORMERR,        /* The message processing rejected the query because #QD==0 */
    
    FP_ARCOUNT_NOT_0            = RCODE_FORMERR,        /* The message processing rejected the query because #AR>0
                                                         * This only makes sense if there is no TSIG nor EDNS support
                                                         * implemented. And even then : is there no conflict with updates ?
                                                         */
    
    FP_QNAME_COMPRESSED         = RCODE_NOTIMP,         /* The message processing found out that the queried name was
                                                         * too long (labels too big or name too big)
                                                         */

    FP_NAME_TOO_LARGE           = RCODE_FORMERR,        /* The message processing found out that the queried name was
                                                         * too long (labels too big or name too big)
                                                         */

	FP_NAME_FORMAT_ERROR		= RCODE_FORMERR,		/* Bad compression of a NAME */
    
    FP_INCORR_PROTO             = RCODE_FORMERR,        /* The message processing found out that something "wrong"
                                                         * was queried (ie: AXFR/IXFR on UDP).
                                                         * THIS NEEDS TO BE CHECKED (xXFR on UDP is not always wrong)
                                                         */
    
    FP_NOT_SUPP_OPC             = RCODE_NOTIMP,         /* The message processing found an unsupported opcode in the
                                                         * query
                                                         */

    FP_XFR_REFUSED              = RCODE_NOTAUTH,        /* Not used yet. Quoted in a TODO in the message processing
                                                         * on the AXFR/IXFR
                                                         * Need to check if it's not already done at another place
                                                         * (closer to the XFR processing)
                                                         */
    
    FP_XFR_UP_TO_DATE           = RCODE_NOERROR,        /* no XFR necessary */
    
    FP_XFR_BROKENZONE           = RCODE_SERVFAIL,       /* the zone is in an invalid state */
    
    FP_XFR_QUERYERROR           = RCODE_FORMERR, 
    
    FP_TSIG_ERROR               = RCODE_NOTAUTH,        /* The message processing handled the included TSIG but rejected
                                                         * it
                                                         */
    
    FP_TSIG_IS_NOT_LAST         = RCODE_FORMERR,        /* There is a record after the TSIG */

    FP_EDNS_BAD_VERSION         = RCODE_BADVERS,        /* Found an EDNS version that is not 0
                                                         * 
                                                         */

    FP_NOT_SUPP_TYPE            = RCODE_FORMERR,        /* AXFR udp query, OPT query */

    FP_SLAVE_NOTIFIES_MASTER    = RCODE_REFUSED,
    
    FP_NONMASTER_NOTIFIES_SLAVE = RCODE_REFUSED,        /* notify from something not in the masters list */
    
    FP_NOTIFY_UNKNOWN_ZONE      = RCODE_NOTAUTH,
    
    FP_UNEXPECTED_RR_IN_QUERY   = RCODE_FORMERR,        /* trash in the packet */
    
    FP_ERROR_READING_QUERY      = RCODE_FORMERR,

    /*
     * Database
     */

    FP_ACCESS_REJECTED          = RCODE_REFUSED,        /* access to the database has been rejected */
    
    FP_NOTIFY_REJECTED          = RCODE_REFUSED,        /* access to the database has been rejected */
    
    FP_NOTIFY_QUERYERROR        = RCODE_FORMERR,

    FP_CLASS_NOTFOUND           = RCODE_REFUSED,        /* class not supported/not in the database */
    
    FP_BASIC_LABEL_NOTFOUND     = RCODE_NXDOMAIN,       /* The label has not been found inside the zone database */

    FP_BASIC_LABEL_DELEGATION   = RCODE_NOERROR,        /* The label was part of a delegation  */

    FP_BASIC_RECORD_NOTFOUND    = RCODE_NOERROR,        /* we didn't found a resource record */

    FP_BASIC_RECORD_FOUND       = RCODE_NOERROR,        /* we found a resource record */
    
    FP_CNAME_LOOP               = RCODE_NOERROR,        /* we detected a loop in the CNAMEs */
    
    FP_CNAME_BROKEN             = RCODE_SERVFAIL,       /* we detected an issue in the database */
    
    FP_CNAME_MAXIMUM_DEPTH      = RCODE_NOERROR,        /* we reached the maximum allowed depth on a CNAME chain */

    FP_NSEC3_RECORD_NOTFOUND    = RCODE_NOERROR,        /* we didn't found a record and we can prove it with nsec3 */

    FP_NSEC3_LABEL_NOTFOUND     = RCODE_NXDOMAIN,       /* we didn't found a domain and we can prove it with nsec3 */

    FP_NSEC_RECORD_NOTFOUND     = RCODE_NOERROR,        /* we didn't found a record and we can prove it with nsec3 */

    FP_NSEC_LABEL_NOTFOUND      = RCODE_NXDOMAIN,       /* we didn't found a domain and we can prove it with nsec3 */

    FP_CANNOT_DYNUPDATE         = RCODE_SERVFAIL,       /* the zone has been frozen (maintenance or admin) and cannot
                                                         * accept an update until being unfrozen
                                                         */
    FP_UPDATE_UNKNOWN_ZONE      = RCODE_NOTZONE,        /* dynupdate on a zone we are not AA of */
    
    FP_NOZONE_FOUND             = RCODE_REFUSED,        /* When a dynamic update does not find the zone it's supposed
                                                         * to update.
                                                         */
            
    FP_INVALID_ZONE             = RCODE_SERVFAIL,
    
    FP_IXFR_UDP                 = RCODE_NOTIMP
};

typedef enum finger_print finger_print;

#ifdef	__cplusplus
}
#endif

#endif	/* _FINGERPRINT_H */
/** @} */

/*----------------------------------------------------------------------------*/

