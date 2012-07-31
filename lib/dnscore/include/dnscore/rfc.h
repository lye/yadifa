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
/** @defgroup 
 *  @ingroup dnscore
 *  @brief 
 *
 *  
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#ifndef RFC_H_
#define RFC_H_

#include <dnscore/sys_types.h>

/*    ------------------------------------------------------------
 *
 *      INCLUDES
 */


/*    ------------------------------------------------------------
 *
 *      VALUES
 */
/* http://en.wikipedia.org/wiki/List_of_DNS_record_types */

/* dns */
#define     DNS_HEADER_LENGTH               12      /*                                    rfc 1035 */
#define     MAX_LABEL_LENGTH                63      /*                                    rfc 1034 */
#define     MAX_DOMAIN_TEXT_LENGTH          (MAX_DOMAIN_LENGTH - 1)     /*                rfc 1034 */
#define     MAX_DOMAIN_LENGTH               255     /*                                    rfc 1034 */
#define     MAX_LABEL_COUNT                 ((MAX_DOMAIN_LENGTH + 1) / 2)
#define     MAX_SOA_RDATA_LENGTH            (255 + 255 + 20)

#define     DNS_DEFAULT_PORT                53

/* edns0 */
#define     EDNS0_MAX_LENGTH                65535    /* See 4.5.5 in RFC                  rfc 2671 */
#define     EDNS0_MIN_LENGTH                512      /*                                   rfc 2671 */
#define     EDNS0_DO                        0        /* DNSSEC OK flag                             */
#define     EDNS0_OPT_0                     0        /* Reserverd                         rfc 2671 */
#define     EDNS0_OPT_3                     3        /* NSID                              rfc 5001 */

/** @todo check this value RFC */
#define     DNSPACKET_MAX_LENGTH            0xffff
#define     UDPPACKET_MAX_LENGTH            512
#define     RDATA_MAX_LENGTH                0xffff


/* dnssec (dns & bind) */
#define     DNSSEC_AD                       0x20     /* Authenticated Data flag                    */
#define     DNSSEC_CD                       0x10     /* Checking Disabled flag                     */

#define     RRSIG_RDATA_HEADER_LEN          18      /* The length of an RRSIG rdata without the
                                                     * signer_name and the signature               */

#define     ID_BITS                         0xFF    /*                                    rfc 1035 */

#define     QR_BITS                         0x80    /*                                    rfc 1035 */
#define     OPCODE_BITS                     0x78    /*                                    rfc 1035 */
#define     OPCODE_SHIFT                    3
#define     AA_BITS                         0x04    /*                                    rfc 1035 */
#define     TC_BITS                         0x02    /*                                    rfc 1035 */
#define     RD_BITS                         0x01    /*                                    rfc 1035 */
#define     RA_BITS                         0x80    /*                                    rfc 1035 */
#define     Z_BITS                          0x40    /*                                    rfc 1035 */
#define     AD_BITS                         0x20    /*                                    rfc 2065 */
#define     CD_BITS                         0x10    /*                                    rfc 2065 */
#define     RCODE_BITS                      0x0F    /*                                    rfc 1035 */

#define     QDCOUNT_BITS                    0xFFFF  /* number of questions                rfc 1035 */
#define     ANCOUNT_BITS                    0xFFFF  /* number of resource records         rfc 1035 */
#define     NSCOUNT_BITS                    0xFFFF  /* name servers in the author.rec.    rfc 1035 */
#define     ARCOUNT_BITS                    0xFFFF  /* additional records                 rfc 1035 */
#define     ZOCOUNT_BITS                    0xFFFF  /* Number of RRs in the Zone Sect.    rfc 2136 */
#define     PRCOUNT_BITS                    0xFFFF  /* Number of RRs in the Prereq. Sect. rfc 2136 */
#define     UPCOUNT_BITS                    0xFFFF  /* Number of RRs in the Upd. Sect.    rfc 2136 */
#define     ADCOUNT_BITS                    0xFFFF  /* Number of RRs in the Add Sect.     rfc 2136 */

#define     OPCODE_QUERY                    (0<<OPCODE_SHIFT)       /* a standard query (QUERY)           rfc 1035 */
#define     OPCODE_IQUERY                   (1<<OPCODE_SHIFT)       /* an inverse query (IQUERY)          rfc 3425 */
#define     OPCODE_STATUS                   (2<<OPCODE_SHIFT)       /* a server status request (STATUS)   rfc 1035 */
#define     OPCODE_NOTIFY                   (4<<OPCODE_SHIFT)       /*                                    rfc 1996 */
#define     OPCODE_UPDATE                   (5<<OPCODE_SHIFT)       /* update                             rfc 2136 */

#define     RCODE_OK                        0       /* No error                           rfc 1035 */
#define     RCODE_NOERROR                   0       /* No error                           rfc 1035 */
#define     RCODE_FE                        1       /* Format error                       rfc 1035 */
#define     RCODE_FORMERR                   1       /* Format error                       rfc 1035 */
#define     RCODE_SF                        2       /* Server failure                     rfc 1035 */
#define     RCODE_SERVFAIL                  2       /* Server failure                     rfc 1035 */
#define     RCODE_NE                        3       /* Name error                         rfc 1035 */
#define     RCODE_NXDOMAIN                  3       /* Name error                         rfc 1035 */
#define     RCODE_NI                        4       /* Not implemented                    rfc 1035 */
#define     RCODE_NOTIMP                    4       /* Not implemented                    rfc 1035 */
#define     RCODE_RE                        5       /* Refused                            rfc 1035 */
#define     RCODE_REFUSED                   5       /* Refused                            rfc 1035 */

#define     RCODE_YXDOMAIN                  6       /* Name exists when it should not     rfc 2136 */
#define     RCODE_YXRRSET                   7       /* RR Set exists when it should not   rfc 2136 */
#define     RCODE_NXRRSET                   8       /* RR set that should exist doesn't   rfc 2136 */
#define     RCODE_NOTAUTH                   9       /* Server not Authortative for zone   rfc 2136 */
#define     RCODE_NOTZONE                   10      /* Name not contained in zone         rfc 2136 */

#define     RCODE_BADVERS                   16      /* Bad OPT Version                    rfc 2671 */
#define     RCODE_BADSIG                    16      /* TSIG Signature Failure             rfc 2845 */
#define     RCODE_BADKEY                    17      /* Key not recognized                 rfc 2845 */
#define     RCODE_BADTIME                   18      /* Signatue out of time window        rfc 2845 */
#define     RCODE_BADMODE                   19      /* Bad TKEY Mode                      rfc 2930 */
#define     RCODE_BADNAME                   20      /* Duplicate key name                 rfc 2930 */
#define     RCODE_BADALG                    21      /* Algorithm not supported            rfc 2930 */
#define     RCODE_BADTRUNC                  22      /* Bad Truncation                     rfc 4635 */

/* EDNS0 */

#define     RCODE_EXT_DNSSEC                0x00800000 /* Network-order, DNSSEC requested */
/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_A                          NU16(1) /* a host address                     rfc 1035 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            NSDNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_NS                         NU16(2) /* an authoritative name server       rfc 1035 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            MADNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MD                         NU16(3) /* CANONIZE - OBSOLETE                rfc 882 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            MADNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MF                         NU16(4) /* CANONIZE - OBSOLETE                rfc 882 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                             CNAME                             /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_CNAME                      NU16(5) /* the canonical name of a alias      rfc 1035 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                             MNAME                             /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                             RNAME                             /    dns formated domain name with
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+    local-part. Can have '\'before .
   |                            SERIAL                             |    32 bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            REFRESH                            |    32 bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             RETRY                             |    32 bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            EXPIRE                             |    32 bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            MINIMUM                            |    32 bit
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_SOA                        NU16(6) /* start of a zone of authority       rfc 1035 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            MADNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MB                         NU16(7) /* CANONIZE - OBSOLETE */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            MMGNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MG                         NU16(8) /* CANONIZE - OBSOLETE */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            NEWNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MR                         NU16(9) /* CANONIZE - OBSOLETE */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            <anything>                         / 
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_NULL                       NU16(10) /* */

/*
     1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |    PROTOCOL   |                                               |
   +-+-+-+-+-+-+-+-+                                               |
   |                                                               |
   /                           <BIT MAP>                           /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    where:

    ADDRESS   - An 32 bit ARPA Internet address

    PROTOCOL  - An 8 bit IP protocol number

    <BIT MAP> - A variable length bit map.  The bit map must be a
    multiple of 8 bits long.
*/
#define     TYPE_WKS                        NU16(11)

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            PTRNAME                            /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_PTR                        NU16(12) /* a domain name pointer              rfc 1035 */

/*
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   /                      CPU                      /    c-string
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   /                       OS                      /    c-string
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   */
#define     TYPE_HINFO                      NU16(13) /* host information                   rfc 1035 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                         RMAILBX                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                         EMAILBX                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_MINFO                      NU16(14) /* mailbox or mail list information   rfc 1035 */

/*
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   |                  PREFERENCE                   |    16 bit
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   /                   EXCHANGE                    /    dns formated domain name
   /                                               /
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   */
#define     TYPE_MX                         NU16(15) /* mail exchange                      rfc 1035 */

/*
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   /                   TXT-DATA                    /    one or more c-strings
   +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
   */
#define     TYPE_TXT                        NU16(16) /* text strings                       rfc 1035 */

#define     TYPE_RP                         NU16(17) /* For Responisble Person             rfc 1183 */
#define     TYPE_ASFDB                      NU16(18) /* CANONIZE */
#define     TYPE_X25                        NU16(19) /*                                    rfc 1183 */
#define     TYPE_ISDN                       NU16(20) /*                                    rfc 1183 */
#define     TYPE_RT                         NU16(21) /*                                    rfc 1183 */
#define     TYPE_NSAP                       NU16(22) /*                                    rfc 1706 */
#define     TYPE_NSAP_PTR                   NU16(23) /*                                    rfc 1348 */
#define     TYPE_SIG                        NU16(24) /* for security signature             rfc 2535 rfc 3755 rfc 4034 */
#define     TYPE_KEY                        NU16(25) /* for security key                   rfc 2535 rfc 3755 rfc 4034 */
#define     TYPE_PX                         NU16(26) /*                                    rfc 2136 */
#define     TYPE_GPOS                       NU16(27) /*                                    rfc 1712 */

/*
   1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                            ADDRESS                            |    32 bit address
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_AAAA                       NU16(28) /* IP6 Address                             rfc 3596 */
#define     TYPE_LOC                        NU16(29) /* Location information                    rfc 1876 */
#define     TYPE_NXT                        NU16(30) /* Location information                    rfc 2535 */
#define     TYPE_EID                        NU16(31) /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_NIMLOC                     NU16(32) /* @note undocumented see draft-lewis-dns-undocumented-types-01 */

#define     TYPE_SRV                        NU16(33) /* Server selection                        rfc 2782 */

#define     TYPE_ATMA                       NU16(34) /* @note undocumented see draft-lewis-dns-undocumented-types-01 */

/*
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                     order                     |    16 bit unsigned integer
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    |                   preference                  |    16 bit unsigned integer
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /                     flags                     /   character-string (a-z0-9) can be empty
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /                   services                    /   character-string (a-z0-9) can be empty
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /                    regexp                     /   character-string
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    /                  replacement                  /   <domain name>
    /                                               /
    +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
    */
#define     TYPE_NAPTR                      NU16(35)      /*                                    rfc 3403 */
#define     TYPE_KX                         NU16(36)      /*                                    rfc 2230 */
#define     TYPE_CERT                       NU16(37)      /*                           rfc 2538 rfc 4398 */
#define     TYPE_A6                         NU16(38)      /* A6                        rfc 2874 rfc 3226 */

/*
   1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
   0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                            DNAME                              /    dns formated domain name
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_DNAME                      NU16(39)      /* CANONIZE                           rfc 2672 */
#define     TYPE_SINK                       NU16(40) /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_OPT                        NU16(41)      /* edns0 flag                         rfc 2671 */
#define     TYPE_APL                        NU16(42)      /*                                    rfc 3123 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Key Tag             |  Algorithm    |  Digest Type  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Digest                             /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_DS                         NU16(43)      /* Delegation Signer                  rfc 3658 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   algorithm   |    fp type    |                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               /
   /                                                               /
   /                          fingerprint                          /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
#define     TYPE_SSHFP                      NU16(44)      /* SSH Key Fingerprint                rfc 4255 */
  
#define     TYPE_IPSECKEY                   NU16(45)      /* IPSECKEY                           rfc 4025 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        Type Covered           |  Algorithm    |     Labels    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         Original TTL                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Signature Expiration                     |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Signature Inception                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |            Key Tag            |                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+         Signer's Name         /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Signature                          /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_RRSIG                      NU16(46)      /* RRSIG                              rfc 4034 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                      Next Domain Name                         /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                       Type Bit Maps                           /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_NSEC                       NU16(47)      /* NSEC                               rfc 3755 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |              Flags            |    Protocol   |   Algorithm   |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Public Key                         /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_DNSKEY                     NU16(48)      /* DNSKEY                             rfc 3755 4034  */

#define     TYPE_DHCID                      NU16(49)      /* DHCID                              rfc 4701 */
#define     TYPE_NSEC3                      NU16(50)      /*                                    rfc 5155 */
#define     TYPE_NSEC3PARAM                 NU16(51)      /*                                    rfc 5155 */

#define     TYPE_HIP                        NU16(55)      /*                                    rfc 5205 */
#define     TYPE_NINFO                      NU16(56)  /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_RKEY                       NU16(57)  /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_TALINK                     NU16(58)  /* @note undocumented see draft-lewis-dns-undocumented-types-01 */


#define     TYPE_SPF                        NU16(99)      /* SPF                                rfc 4408 */

#define     TYPE_UINFO                      NU16(100)
#define     TYPE_UID                        NU16(101)
#define     TYPE_GID                        NU16(102)
#define     TYPE_UNSPEC                     NU16(103)

#define     TYPE_TKEY                       NU16(249)     /* Transaction Key                    rfc 2930 */
#define     TYPE_TSIG                       NU16(250)     /* Transaction Signature              rfc 2845 */
#define     TYPE_IXFR                       NU16(251)     /* Incremental Transfer               rfc 1995 */
#define     TYPE_AXFR                       NU16(252)     /* Transfer of an entire zone         rfc 1035 */
#define     TYPE_MAILB                      NU16(253)     /* A request for mailbox-related records (MB, MG or MR) rfc 1035 */
#define     TYPE_MAILA                      NU16(254)     /* A request for mail agent RRs (Obsolete - see MX) rfc 1035 */
#define     TYPE_ANY                        NU16(255)     /* a request for all records          rfc 1035 */
#define     TYPE_URI                        NU16(256)    /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_CAA                        NU16(257)    /* @note undocumented see draft-lewis-dns-undocumented-types-01 */

#define     TYPE_TA                         NU16(32768)  /* @note undocumented see draft-lewis-dns-undocumented-types-01 */

/*
    1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           Key Tag             |  Algorithm    |  Digest Type  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   /                                                               /
   /                            Digest                             /
   /                                                               /
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   */
#define     TYPE_DLV                        NU16(32769)   /*                                     rfc 4431 */

#define     TYPE_PRIVATE_FIRST              NU16(65280)
#define     TYPE_PRIVATE_LAST               NU16(65534)

#define     HOST_CLASS_IN                   1             /* the Internet                      rfc 1025 */

#define     CLASS_IN                        NU16(HOST_CLASS_IN) /* the Internet                rfc 1025 */
#define     CLASS_CS                        NU16(2)       /* CSNET class                       rfc 1025 */
#define     CLASS_CH                        NU16(3)       /* the CHAOS class                   rfc 1025 */
#define     CLASS_HS                        NU16(4)       /* Hesiod                            rfc 1025 */
#define     CLASS_CTRL                      NU16(0x2A)    /* @note Yadifa controller class */
#define     CLASS_NONE                      NU16(254)     /* rfc 2136                          rfc 2136 */
#define     CLASS_ANY                       NU16(255)     /* rfc 1035  QCLASS ONLY             rfc 1025 */


/* -----------------------------------------------------------------*/

#define     AXFR_TSIG_PERIOD                100

/* -----------------------------------------------------------------*/

#define     DNSKEY_FLAG_KEYSIGNINGKEY       0x0001
#define     DNSKEY_FLAG_ZONEKEY             0x0100

#define     DNSKEY_PROTOCOL_FIELD           3           /* MUST be this */

#define     DNSKEY_ALGORITHM_DSASHA1        3
#define     DNSKEY_ALGORITHM_RSASHA1        5
#define     DNSKEY_ALGORITHM_DSASHA1_NSEC3  6
#define     DNSKEY_ALGORITHM_RSASHA1_NSEC3  7

#define     NSEC3_FLAGS_OPTOUT              1           /*  */

/* -----------------------------------------------------------------*/

#define     IS_TYPE_PRIVATE(t)              ((t)>=65280&&((t)<=65534))

/*    ------------------------------------------------------------
 *
 *      STRUCTS
 */

#define EDNS0_RECORD_SIZE                   11

/* rfc 2671 */
struct edns0_data
{
    u8                 domain_name;    /* must be empty            */
    u16                        opt;
    u16               payload_size;
    u8              extended_rcode;    /* extended rcode and flags */
    u8                     version;    /* extended rcode and flags */
    u8                      z_bits;    /* extended rcode and flags */
    u8                 option_code;
    u16              option_length;
};

typedef struct edns0_data edns0_data;

/* - */

typedef struct value_name_table value_name_table;

struct value_name_table
{
    u32                        id;
    char                    *data;
};


typedef value_name_table class_table;
typedef value_name_table type_table;

typedef struct message_header message_header;

struct message_header
{
    u16                         id;
    u16                     opcode;
    u16                    qdcount;
    u16                    ancount;
    u16                    nscount;
    u16                    arcount;
};

/*    ------------------------------------------------------------    */

#define     CLASS_IN_NAME                   "IN"
#define     CLASS_CS_NAME                   "CS"
#define     CLASS_CH_NAME                   "CH"
#define     CLASS_HS_NAME                   "HS"
#define     CLASS_CTRL_NAME                 "CTRL"  /* @note YADIFA's personal class, maybe one day in a RFC */
#define     CLASS_NONE_NAME                 "NONE"
#define     CLASS_ANY_NAME                  "ANY"

extern const class_table qclass[];

#define     TYPE_A_NAME                     "A"
#define     TYPE_NS_NAME                    "NS"
#define     TYPE_MD_NAME                    "MD"
#define     TYPE_MF_NAME                    "MF"
#define     TYPE_CNAME_NAME                 "CNAME"
#define     TYPE_SOA_NAME                   "SOA"
#define     TYPE_MB_NAME                    "MB"
#define     TYPE_MG_NAME                    "MG"
#define     TYPE_MR_NAME                    "MR"
#define     TYPE_NULL_NAME                  "NULL"
#define     TYPE_WKS_NAME                   "WKS"
#define     TYPE_PTR_NAME                   "PTR"
#define     TYPE_HINFO_NAME                 "HINFO"
#define     TYPE_MINFO_NAME                 "MINFO"
#define     TYPE_MX_NAME                    "MX"
#define     TYPE_TXT_NAME                   "TXT"
#define     TYPE_RP_NAME                    "RP"
#define     TYPE_ASFDB_NAME                 "ASFDB"
#define     TYPE_X25_NAME                   "X25"
#define     TYPE_ISDN_NAME                  "ISDN"
#define     TYPE_RT_NAME                    "RT"
#define     TYPE_NSAP_NAME                  "NSAP"
#define     TYPE_NSAP_PTR_NAME              "NSAP-PTR"
#define     TYPE_SIG_NAME                   "SIG"
#define     TYPE_KEY_NAME                   "KEY"
#define     TYPE_PX_NAME                    "PX"
#define     TYPE_GPOS_NAME                  "GPOS"
#define     TYPE_AAAA_NAME                  "AAAA"
#define     TYPE_LOC_NAME                   "LOC"
#define     TYPE_NXT_NAME                   "NXT"
#define     TYPE_EID_NAME                   "EID"       /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_NIMLOC_NAME                "NIMLOC"    /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_SRV_NAME                   "SRV"
#define     TYPE_ATMA_NAME                  "ATMA"
#define     TYPE_NAPTR_NAME                 "NAPTR"
#define     TYPE_KX_NAME                    "KX"
#define     TYPE_CERT_NAME                  "CERT"
#define     TYPE_A6_NAME                    "A6"
#define     TYPE_DNAME_NAME                 "DNAME"
#define     TYPE_SINK_NAME                  "SINK"      /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_OPT_NAME                   "OPT"
#define     TYPE_APL_NAME                   "APL"
#define     TYPE_DS_NAME                    "DS"
#define     TYPE_SSHFP_NAME                 "SSHFP"
#define     TYPE_IPSECKEY_NAME              "IPSECKEY"
#define     TYPE_RRSIG_NAME                 "RRSIG"
#define     TYPE_NSEC_NAME                  "NSEC"
#define     TYPE_DNSKEY_NAME                "DNSKEY"
#define     TYPE_DHCID_NAME                 "DHCID"
#define     TYPE_NSEC3_NAME                 "NSEC3"
#define     TYPE_NSEC3PARAM_NAME            "NSEC3PARAM"
#define     TYPE_HIP_NAME                   "HIP"
#define     TYPE_NINFO_NAME                 "NINFO"     /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_RKEY_NAME                  "RKEY"      /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_TALINK_NAME                "TALINK"    /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_CDS_NAME                   "CDS"       /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_SPF_NAME                   "SPF"
#define     TYPE_UINFO_NAME                 "UINFO"
#define     TYPE_UID_NAME                   "UID"
#define     TYPE_GID_NAME                   "GID"
#define     TYPE_UNSPEC_NAME                "UNSPEC"
#define     TYPE_TKEY_NAME                  "TKEY"
#define     TYPE_TSIG_NAME                  "TSIG"
#define     TYPE_IXFR_NAME                  "IXFR"
#define     TYPE_AXFR_NAME                  "AXFR"
#define     TYPE_MAILB_NAME                 "MAILB"
#define     TYPE_MAILA_NAME                 "MAILA"
#define     TYPE_ANY_NAME                   "ANY"  /** @note type ANY's string was set to '*' ? 
                                                    *  Setting this to anything else will break
                                                    *        dnsformat:358
                                                    */
#define     TYPE_URI_NAME                   "URI"       /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_CAA_NAME                   "CAA"       /* @note undocumented see draft-lewis-dns-undocumented-types-01 */

#define     TYPE_TA_NAME                    "TA"        /* @note undocumented see draft-lewis-dns-undocumented-types-01 */
#define     TYPE_DLV_NAME                   "DLV"

extern const type_table qtype[];

/**
 * Static asciiz representation of a dns class
 * 
 * @param c
 * @return the c-string
 */

const char*
get_name_from_class(u16 c);

/**
 * Static asciiz representation of a dns type
 * 
 * @param c
 * @return the c-string
 */

const char*
get_name_from_type(u16 t);

/** \brief Get the numeric value of a class (network order) from its name
 *
 *  @param[in]  src the name of the class
 *  @param[out] dst value of the class, network order
 *
 *  @retval OK
 *  @retval NOK
 */
int
get_class_from_name(const char *src, u16 *dst);

/** \brief Get the numeric value of a class (network order) from its name
 *  Case insensitive
 *
 *  @param[in]  src the name of the class (case insensitive)
 *  @param[out] dst value of the class, network order
 *
 *  @retval OK
 *  @retval NOK
 */
int
get_class_from_case_name(const char *src, u16 *dst);

/** \brief Get the numeric value of a type (network order) from its name
 *
 *  @param[in]  src the name of the type
 *  @param[out] dst value of the type, network order
 *
 *  @retval OK
 *  @retval NOK
 */
int
get_type_from_name(const char *src, u16 *dst);

/** \brief Get the numeric value of a type (network order) from its name
 *  Case insensitive
 *
 *  @param[in]  src the name of the type (case insensitive)
 *  @param[out] dst value of the type, network order
 *
 *  @retval OK
 *  @retval NOK
 */
int
get_type_from_case_name(const char *src, u16 *dst);

/**
 * @brief Case-insensitive search for the name in the table, returns the value
 * 
 * @param table the name->value table
 * @param name the name to look for
 * @param out_value a pointer to an u32 that will hold the value in case of a match
 * 
 * @return SUCCESS iff the name was matched
 */
ya_result
get_value_from_casename(value_name_table *table, const char *name, u32 *out_value);

#endif /* RFC_H_ */

/** @} */

/*----------------------------------------------------------------------------*/

