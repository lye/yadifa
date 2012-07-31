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
/** @defgroup format C-string formatting
 *  @ingroup dnscore
 *  @brief
 *
 *
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>	/* Required for BSD */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "dnscore/rfc.h"
#include "dnscore/ctrl-rfc.h"
#include "dnscore/format.h"
#include "dnscore/dnsname.h"
#include "dnscore/base32hex.h"
#include "dnscore/dnsformat.h"
#include "dnscore/host_address.h"

#define NULL_STRING_SUBSTITUTE (u8*)"(NULL)"
#define NULL_STRING_SUBSTITUTE_LEN 6 /*(sizeof(NULL_STRING_SUBSTITUTE)-1)*/

/** digest32h
 *
 *  @note The digest format is 1 byte of length, n bytes of digest data.
 */

static void
digest32h_format_handler_method(void *restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void * restrict reserved_for_method_parameters)
{
    u8 *digest = (u8 *)val;

    if(digest != NULL)
    {
        output_stream_write_base32hex(stream, &digest[1], *digest);
    }
    else
    {
        format_asciiz("NULL", stream, padding, pad_char, left_justified);
    }
}

static format_handler_descriptor digest32h_format_handler_descriptor ={
    "digest32h",
    9,
    digest32h_format_handler_method
};

/* dnsname */

static void
dnsname_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    const u8 dot = (u8)'.';
    u8 *label    = (u8*)val;

    if(label != NULL)
    {
        u8 label_len;

        FORMAT_BREAK_ON_INVALID(label, 1);

        label_len = *label;

        if(label_len > 0)
        {
            do
            {
                FORMAT_BREAK_ON_INVALID(&label[1], label_len);

                output_stream_write(stream, ++label, label_len);

                output_stream_write(stream, &dot, 1);

                label += label_len;

                FORMAT_BREAK_ON_INVALID(label, 1);

                label_len = *label;

            }
            while(label_len > 0);
        }
        else
        {
            output_stream_write(stream, &dot, 1);
        }
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnsname_format_handler_descriptor ={
    "dnsname",
    7,
    dnsname_format_handler_method
};

/*  */

static void
dnsnamevector_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    u8 dot                     = '.';
    dnsname_vector* namevector = (dnsname_vector *)val;

    if(namevector != NULL)
    {
        FORMAT_BREAK_ON_INVALID(namevector, sizeof(dnsname_vector));

        s32 top = namevector->size;

        if(top >= 0)
        {
            s32 idx;

            for(idx = 0; idx <= top; idx++)
            {
                u8 *label = namevector->labels[idx];

                FORMAT_BREAK_ON_INVALID(label, 1);

                u8 label_len = *label++;

                FORMAT_BREAK_ON_INVALID(label, label_len);

                output_stream_write(stream, label, label_len);
                output_stream_write(stream, &dot, 1);
            }
        }
        else
        {
            output_stream_write(stream, &dot, 1);
        }
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnsnamevector_format_handler_descriptor ={
    "dnsnamevector",
    13,
    dnsnamevector_format_handler_method
};

/* dnsnamestack */

static void
dnsnamestack_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    u8 dot                   = '.';
    dnsname_stack *namestack = (dnsname_stack *)val;

    if(namestack != NULL)
    {
        s32 top = namestack->size;

        if(top >= 0)
        {
            do
            {
                u8 *label    = namestack->labels[top];
                u8 label_len = *label++;
                output_stream_write(stream, label, label_len);
                output_stream_write(stream, &dot, 1);
            }
            while(--top >= 0);
        }
        else
        {
            output_stream_write(stream, &dot, 1);
        }
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnsnamestack_format_handler_descriptor ={
    "dnsnamestack",
    12,
    dnsnamestack_format_handler_method
};

/* dnslabel */

static void
dnslabel_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    u8 *label = (u8 *)val;

    if(label != NULL)
    {
        u8 label_len = *label++;
        output_stream_write(stream, label, label_len);
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnslabel_format_handler_descriptor ={
    "dnslabel",
    8,
    dnslabel_format_handler_method
};

/* class */

static void
dnsclass_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    u16 *classp = (u16 *)val;


    if(classp != NULL)
    {
        const char *txt = NULL;
        s32 len = 0;

        switch(GET_U16_AT(*classp))
        {
            case CLASS_IN:
                len = 2;
                txt = CLASS_IN_NAME;
                break;
            case CLASS_HS:
                len = 2;
                txt = CLASS_HS_NAME;
                break;
            case CLASS_CH:
                len = 2;
                txt = CLASS_CH_NAME;
                break;
            case CLASS_CTRL:
                len = 4;
                txt = CLASS_CTRL_NAME;
                break;
            case CLASS_ANY:
                len = 3;
                txt = CLASS_ANY_NAME;
                break;
            default:
                output_stream_write(stream, (u8 *)"CLASS", 5); /* rfc 3597 */
                format_dec_u64((u64) ntohs(*classp), stream, 0, ' ', TRUE);
                return;
        }

        output_stream_write(stream, (u8 *)txt, len);
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnsclass_format_handler_descriptor ={
    "dnsclass",
    8,
    dnsclass_format_handler_method
};

/* type */

static void
dnstype_format_handler_method(void *val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *reserved_for_method_parameters)
{
    u16 *typep = (u16 *)val;

    if(typep != NULL)
    {
        const char *txt = NULL;
        s32 len         = 0;

        switch(GET_U16_AT(*typep))
        {
            case TYPE_A:
                len = 1;
                txt = TYPE_A_NAME;
                break;
            case TYPE_NS:
                len = 2;
                txt = TYPE_NS_NAME;
                break;
            case TYPE_MD:
                len = 2;
                txt = TYPE_MD_NAME;
                break;
            case TYPE_MF:
                len = 2;
                txt = TYPE_MF_NAME;
                break;
            case TYPE_CNAME:
                len = 5;
                txt = TYPE_CNAME_NAME;
                break;
            case TYPE_SOA:
                len = 3;
                txt = TYPE_SOA_NAME;
                break;
            case TYPE_MB:
                len = 2;
                txt = TYPE_MB_NAME;
                break;
            case TYPE_MG:
                len = 2;
                txt = TYPE_MG_NAME;
                break;
            case TYPE_MR:
                len = 2;
                txt = TYPE_MR_NAME;
                break;
            case TYPE_NULL:
                len = 4;
                txt = TYPE_NULL_NAME;
                break;
            case TYPE_WKS:
                len = 3;
                txt = TYPE_WKS_NAME;
                break;
            case TYPE_PTR:
                len = 3;
                txt = TYPE_PTR_NAME;
                break;
            case TYPE_HINFO:
                len = 5;
                txt = TYPE_HINFO_NAME;
                break;
            case TYPE_MINFO:
                len = 5;
                txt = TYPE_MINFO_NAME;
                break;
            case TYPE_MX:
                len = 2;
                txt = TYPE_MX_NAME;
                break;
            case TYPE_TXT:
                len = 3;
                txt = TYPE_TXT_NAME;
                break;
            case TYPE_RP:
                len = 2;
                txt = TYPE_RP_NAME;
                break;
            case TYPE_ASFDB:
                len = 5;
                txt = TYPE_ASFDB_NAME;
                break;
            case TYPE_X25:
                len = 3;
                txt = TYPE_X25_NAME;
                break;
            case TYPE_ISDN:
                len = 4;
                txt = TYPE_ISDN_NAME;
                break;
            case TYPE_RT:
                len = 2;
                txt = TYPE_RT_NAME;
                break;
            case TYPE_NSAP:
                len = 4;
                txt = TYPE_NSAP_NAME;
                break;
            case TYPE_NSAP_PTR:
                len = 8;
                txt = TYPE_NSAP_PTR_NAME;
                break;
            case TYPE_SIG:
                len = 3;
                txt = TYPE_SIG_NAME;
                break;
            case TYPE_KEY:
                len = 3;
                txt = TYPE_KEY_NAME;
                break;
            case TYPE_PX:
                len = 2;
                txt = TYPE_PX_NAME;
                break;
            case TYPE_GPOS:
                len = 4;
                txt = TYPE_GPOS_NAME;
                break;
            case TYPE_AAAA:
                len = 4;
                txt = TYPE_AAAA_NAME;
                break;
            case TYPE_LOC:
                len = 3;
                txt = TYPE_LOC_NAME;
                break;
            case TYPE_NXT:
                len = 3;
                txt = TYPE_NXT_NAME;
                break;
            case TYPE_EID:
                len = 3;
                txt = TYPE_EID_NAME;
                break;
            case TYPE_NIMLOC:
                len = 6;
                txt = TYPE_NIMLOC_NAME;
                break;
            case TYPE_SRV:
                len = 3;
                txt = TYPE_SRV_NAME;
                break;
            case TYPE_ATMA:
                len = 4;
                txt = TYPE_ATMA_NAME;
                break;
            case TYPE_NAPTR:
                len = 5;
                txt = TYPE_NAPTR_NAME;
                break;
            case TYPE_KX:
                len = 2;
                txt = TYPE_KX_NAME;
                break;
            case TYPE_CERT:
                len = 4;
                txt = TYPE_CERT_NAME;
                break;
            case TYPE_A6:
                len = 2;
                txt = TYPE_A6_NAME;
                break;
            case TYPE_DNAME:
                len = 5;
                txt = TYPE_DNAME_NAME;
                break;
            case TYPE_SINK:
                len = 4;
                txt = TYPE_SINK_NAME;
                break;
            case TYPE_OPT:
                len = 3;
                txt = TYPE_OPT_NAME;
                break;
            case TYPE_APL:
                len = 3;
                txt = TYPE_APL_NAME;
                break;
            case TYPE_DS:
                len = 2;
                txt = TYPE_DS_NAME;
                break;
            case TYPE_SSHFP:
                len = 5;
                txt = TYPE_SSHFP_NAME;
                break;
            case TYPE_IPSECKEY:
                len = 8;
                txt = TYPE_IPSECKEY_NAME;
                break;
            case TYPE_RRSIG:
                len = 5;
                txt = TYPE_RRSIG_NAME;
                break;
            case TYPE_NSEC:
                len = 4;
                txt = TYPE_NSEC_NAME;
                break;
            case TYPE_DNSKEY:
                len = 6;
                txt = TYPE_DNSKEY_NAME;
                break;
            case TYPE_DHCID:
                len = 5;
                txt = TYPE_DHCID_NAME;
                break;
            case TYPE_NSEC3:
                len = 5;
                txt = TYPE_NSEC3_NAME;
                break;
            case TYPE_NSEC3PARAM:
                len = 10;
                txt = TYPE_NSEC3PARAM_NAME;
                break;
            case TYPE_HIP:
                len = 3;
                txt = TYPE_HIP_NAME;
                break;
            case TYPE_NINFO:
                len = 5;
                txt = TYPE_NINFO_NAME;
                break;
            case TYPE_RKEY:
                len = 4;
                txt = TYPE_RKEY_NAME;
                break;
            case TYPE_TALINK:
                len = 6;
                txt = TYPE_TALINK_NAME;
                break;
            case TYPE_SPF:
                len = 3;
                txt = TYPE_SPF_NAME;
                break;
            case TYPE_UINFO:
                len = 5;
                txt = TYPE_UINFO_NAME;
                break;
            case TYPE_UID:
                len = 3;
                txt = TYPE_UID_NAME;
                break;
            case TYPE_GID:
                len = 3;
                txt = TYPE_GID_NAME;
                break;
            case TYPE_UNSPEC:
                len = 5;
                txt = TYPE_UNSPEC_NAME;
                break;
            case TYPE_TKEY:
                len = 4;
                txt = TYPE_TKEY_NAME;
                break;
            case TYPE_TSIG:
                len = 4;
                txt = TYPE_TSIG_NAME;
                break;
            case TYPE_IXFR:
                len = 4;
                txt = TYPE_IXFR_NAME;
                break;
            case TYPE_AXFR:
                len = 4;
                txt = TYPE_AXFR_NAME;
                break;
            case TYPE_MAILB:
                len = 5;
                txt = TYPE_MAILB_NAME;
                break;
            case TYPE_MAILA:
                len = 5;
                txt = TYPE_MAILA_NAME;
                break;
            case TYPE_ANY:
                len = 3;
                txt = TYPE_ANY_NAME;
                break;
            case TYPE_URI:
                len = 3;
                txt = TYPE_URI_NAME;
                break;
            case TYPE_CAA:
                len = 3;
                txt = TYPE_CAA_NAME;
                break;
            case TYPE_TA:
                len = 2;
                txt = TYPE_TA_NAME;
                break;
            case TYPE_DLV:
                len = 3;
                txt = TYPE_DLV_NAME;
                break;
            case TYPE_ZONE_TYPE:
                len = 8;
                txt = TYPE_ZONE_TYPE_NAME;
                break;
            case TYPE_ZONE_FILE:
                len = 8;
                txt = TYPE_ZONE_FILE_NAME;
                break;
            case TYPE_ZONE_ALSO_NOTIFY:
                len = 9;
                txt = TYPE_ZONE_ALSO_NOTIFY_NAME;
                break;
            case TYPE_ZONE_MASTER:
                len = 6;
                txt = TYPE_ZONE_MASTER_NAME;
                break;
            case TYPE_ZONE_DNSSEC:
                len = 6;
                txt = TYPE_ZONE_DNSSEC_NAME;
                break;
            case TYPE_CTRL_SHUTDOWN:
                len = 8;
                txt = TYPE_CTRL_SHUTDOWN_NAME;
                break;
            case TYPE_CTRL_ZONEFREEZE:
                len = 6;
                txt = TYPE_CTRL_ZONEFREEZE_NAME;
                break;
            case TYPE_CTRL_ZONEUNFREEZE:
                len = 8;
                txt = TYPE_CTRL_ZONEUNFREEZE_NAME;
                break;
            case TYPE_CTRL_ZONERELOAD:
                len = 6;
                txt = TYPE_CTRL_ZONERELOAD_NAME;
                break;
            case TYPE_CTRL_LOGREOPEN:
                len = 9;
                txt = TYPE_CTRL_LOGREOPEN_NAME;
                break;
            case TYPE_CTRL_CFGMERGE:
                len = 8;
                txt = TYPE_CTRL_CFGMERGE_NAME;
                break;
            case TYPE_CTRL_CFGSAVE:
                len = 7;
                txt = TYPE_CTRL_CFGSAVE_NAME;
                break;
            case TYPE_CTRL_CFGLOAD:
                len = 7;
                txt = TYPE_CTRL_CFGLOAD_NAME;
                break;
            default:
                output_stream_write(stream, (u8 *)"TYPE", 4); /* rfc 3597 */
                format_dec_u64((u64) ntohs(*typep), stream, 0, ' ', TRUE);
                return;
        }

        output_stream_write(stream, (u8 *)txt, len);
    }
    else
    {
        output_stream_write(stream, NULL_STRING_SUBSTITUTE, NULL_STRING_SUBSTITUTE_LEN);
    }
}

static format_handler_descriptor dnstype_format_handler_descriptor ={
    "dnstype",
    7,
    dnstype_format_handler_method
};

static void
sockaddr_format_handler_method(void *restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *restrict reserved_for_method_parameters)
{
    char buffer[INET6_ADDRSTRLEN + 1 + 5 + 1];
    char *src           = buffer;
    
    struct sockaddr *sa = (struct sockaddr*)val;

    switch(sa->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)sa;
            
            if(inet_ntop(ipv4->sin_family, &ipv4->sin_addr, buffer, sizeof (buffer)) != NULL)
            {
                int n       = strlen(buffer);
                buffer[n++] = ':';
                snprintf(&buffer[n],6, "%i", ntohs(ipv4->sin_port));
            }
            else
            {
                src = strerror(errno);
            }
            break;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)sa;

            if(inet_ntop(ipv6->sin6_family, &ipv6->sin6_addr, buffer, sizeof (buffer)) != NULL)
            {
                int n       = strlen(buffer);
                buffer[n++] = ':';
                snprintf(&buffer[n],6, "%i", ntohs(ipv6->sin6_port));
            }
            else
            {
                src = strerror(errno);
            }
            break;
        }
        default:
        {
            snprintf(buffer, sizeof (buffer), "AF_#%i:?", sa->sa_family);
            break;
        }
    }

    format_asciiz(src, stream, padding, pad_char, left_justified);
}

static format_handler_descriptor sockaddr_format_handler_descriptor ={
    "sockaddr",
    8,
    sockaddr_format_handler_method
};

static void
hostaddr_format_handler_method(void *restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void *restrict reserved_for_method_parameters)
{
    char buffer[INET6_ADDRSTRLEN + 1 + 5 + 1];
    char *src = buffer;

    if(val != NULL)
    {
        host_address *ha = (struct host_address*)val;

        switch(ha->version)
        {
            case HOST_ADDRESS_IPV4:
            {
                if(inet_ntop(AF_INET, ha->ip.v4.bytes, buffer, sizeof (buffer)) != NULL)
                {
                    int n       = strlen(buffer);
                    buffer[n++] = ':';
                    snprintf(&buffer[n],6, "%i", ntohs(ha->port));
                }
                else
                {
                    src = strerror(errno);
                }
                break;
            }
            case HOST_ADDRESS_IPV6:
            {
                if(inet_ntop(AF_INET6, ha->ip.v6.bytes, buffer, sizeof (buffer)) != NULL)
                {
                    int n       = strlen(buffer);
                    buffer[n++] = ':';
                    snprintf(&buffer[n],6, "%i", ntohs(ha->port));
                }
                else
                {
                    src = strerror(errno);
                }
                break;
            }
            case HOST_ADDRESS_DNAME:
            {
                buffer[0] = '\0';
                dnsname_to_cstr(buffer, ha->ip.dname.dname);
            }
            default:
            {
                src = "INVALID";
                break;
            }
        }
    }
    else
    {
        src = "NULL";
    }

    format_asciiz(src, stream, padding, pad_char, left_justified);
}

static format_handler_descriptor hostaddr_format_handler_descriptor ={
    "hostaddr",
    8,
    hostaddr_format_handler_method
};

static void
sockaddrip_format_handler_method(void *restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void * restrict reserved_for_method_parameters)
{
    char buffer[INET6_ADDRSTRLEN + 1 + 5 + 1];
    char *src           = buffer;

    struct sockaddr *sa = (struct sockaddr*)val;

    switch(sa->sa_family)
    {
        case AF_INET:
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)sa;

            if(inet_ntop(ipv4->sin_family, &ipv4->sin_addr, buffer, sizeof (buffer)) != NULL)
            {
            }
            else
            {
                src = strerror(errno);
            }
            break;
        }
        case AF_INET6:
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)sa;

            if(inet_ntop(ipv6->sin6_family, &ipv6->sin6_addr, buffer, sizeof (buffer)) != NULL)
            {
            }
            else
            {
                src = strerror(errno);
            }
            break;
        }
        default:
        {
            snprintf(buffer, sizeof (buffer), "AF_#%i:?", sa->sa_family);
            break;
        }
    }

    format_asciiz(src, stream, padding, pad_char, left_justified);
}

static format_handler_descriptor sockaddrip_format_handler_descriptor ={
    "sockaddrip",
    10,
    sockaddrip_format_handler_method
};

static void
rdatadesc_format_handler_method(void *restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void * restrict reserved_for_method_parameters)
{
    rdata_desc *desc = (rdata_desc *)val;
    osprint_rdata(stream, desc->type, desc->rdata, desc->len);
}

static format_handler_descriptor rdatadesc_format_handler_descriptor ={
    "rdatadesc",
    9,
    rdatadesc_format_handler_method
};

static void
typerdatadesc_format_handler_method(void * restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void * restrict reserved_for_method_parameters)
{
    rdata_desc *desc = (rdata_desc*)val;
    dnstype_format_handler_method(&desc->type, stream, padding, pad_char, left_justified, reserved_for_method_parameters);
    output_stream_write_u8(stream, (u8)pad_char);
    osprint_rdata(stream, desc->type, desc->rdata, desc->len);
}

static format_handler_descriptor typerdatadesc_format_handler_descriptor = {
    "typerdatadesc",
    13,
    typerdatadesc_format_handler_method
};


static void
recordwire_format_handler_method(void * restrict val, output_stream *stream, s32 padding, char pad_char, bool left_justified, void * restrict reserved_for_method_parameters)
{
    u8      *domain = (u8 *)val;
    u32 domain_len  = dnsname_len(domain);

    u16    *typeptr = (u16 *)&domain[domain_len];
    u16   *classptr = (u16 *)&domain[domain_len + 2];
    u32         ttl = ntohl(GET_U32_AT(domain[domain_len + 4]));
    u16   rdata_len = ntohs(GET_U16_AT(domain[domain_len + 8]));
    u8       *rdata = &domain[domain_len + 10];

    dnsname_format_handler_method(domain, stream, padding, pad_char, left_justified, reserved_for_method_parameters);
    output_stream_write_u8(stream, (u8)pad_char);

    dnsclass_format_handler_method(classptr, stream, padding, pad_char, left_justified, reserved_for_method_parameters);
    output_stream_write_u8(stream, (u8)pad_char);

    dnstype_format_handler_method(typeptr, stream, padding, pad_char, left_justified, reserved_for_method_parameters);
    output_stream_write_u8(stream, (u8)pad_char);
    
    osprint_rdata(stream, *typeptr, rdata, rdata_len);
}

static format_handler_descriptor recordwire_format_handler_descriptor = {
    "recordwire",
    10,
    recordwire_format_handler_method
};

static bool dnsformat_class_init_done = FALSE;

void
dnsformat_class_init()
{
    if(dnsformat_class_init_done)
    {
        return;
    }

    dnsformat_class_init_done = TRUE;

    format_class_init();

    format_registerclass(&dnsname_format_handler_descriptor);
    format_registerclass(&dnslabel_format_handler_descriptor);
    format_registerclass(&dnsclass_format_handler_descriptor);
    format_registerclass(&dnstype_format_handler_descriptor);
    format_registerclass(&dnsnamestack_format_handler_descriptor);
    format_registerclass(&dnsnamevector_format_handler_descriptor);
    format_registerclass(&digest32h_format_handler_descriptor);
    format_registerclass(&sockaddr_format_handler_descriptor);
    format_registerclass(&sockaddrip_format_handler_descriptor);
    format_registerclass(&hostaddr_format_handler_descriptor);
    format_registerclass(&rdatadesc_format_handler_descriptor);
    format_registerclass(&typerdatadesc_format_handler_descriptor);
    format_registerclass(&recordwire_format_handler_descriptor);
}

/** @} */

/*----------------------------------------------------------------------------*/

