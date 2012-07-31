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
/** @defgroup dnscoreerror Error
 *  @ingroup dnscore
 *  @brief
 *
 *
 *
 * @{
 *
 *----------------------------------------------------------------------------*/

#include "dnscore/sys_types.h"
#include "dnscore/sys_error.h"
#include "dnscore/rfc.h"

#define ERRORTBL_TAG 0x4c4254524f525245

/*----------------------------------------------------------------------------*/

void
dief(ya_result error_code, const char* format, ...)
{
    /**
     * @note Cannot use format here.  The output call HAS to be from the standard library/system.
     */
    fflush(NULL);
    fprintf(stderr, "Critical error : %i %x '%s'\n", error_code, error_code, error_gettext(error_code));
    fflush(NULL);
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args); /* Keep native */
    va_end(args);
    fflush(NULL);
    exit(EXIT_FAILURE);
}

#define ERROR_TABLE_SIZE_INCREMENT 32

static value_name_table* error_table = NULL;
static u32 error_table_count = 0;
static u32 error_table_size = 0;

void
error_unregister_all()
{
    value_name_table* table = error_table;
    value_name_table* entry = error_table;
    value_name_table* limit = &error_table[error_table_count];

    error_table = NULL;
    error_table_count = 0;
    error_table_size = 0;

    while(entry < limit)
    {
        free(entry->data);
        entry++;
    }

    free(table);
}

void
error_register(ya_result code, const char* text)
{
    if(text == NULL)
    {
        text = "NULL";
    }

    if((code & 0xffff0000) == ERRNO_ERROR_BASE)
    {
        fprintf(stderr, "error_register(%08x,%s) : the errno space is reserved (0x8000xxxx)", code, text);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    if(error_table_count == error_table_size)
    {
        error_table_size += ERROR_TABLE_SIZE_INCREMENT;
        REALLOC_OR_DIE(value_name_table*, error_table, error_table_size * sizeof (value_name_table), ERRORTBL_TAG);
    }

    error_table[error_table_count].id = code;
    error_table[error_table_count].data = strdup(text);

    error_table_count++;
}

static char error_gettext_tmp[64];

const char*
error_gettext(ya_result code)
{
    /* errno handling */

    if(code > 0)
    {
        snprintf(error_gettext_tmp, sizeof (error_gettext_tmp), "Success (%08x)", code);
        return error_gettext_tmp;
    }

    if((code & 0xffff0000) == ERRNO_ERROR_BASE)
    {
        return strerror(code & 0xffff);
    }

    /**/

    for(u32 idx = 0; idx < error_table_count; idx++)
    {
        if(error_table[idx].id == code)
        {
            return error_table[idx].data;
        }
    }

    u32 error_base = code & 0xffff0000;

    for(u32 idx = 0; idx < error_table_count; idx++)
    {
        if(error_table[idx].id == error_base)
        {
            return error_table[idx].data;
        }
    }

    snprintf(error_gettext_tmp, sizeof (error_gettext_tmp), "Undefined error code %08x", code);

    return error_gettext_tmp;
}

static bool dnscore_register_errors_done = FALSE;

void
dnscore_register_errors()
{
    if(dnscore_register_errors_done)
    {
        return;
    }

    dnscore_register_errors_done = TRUE;

    error_register(SUCCESS, "Success");
    error_register(SERVER_ERROR_BASE, "SERVER_ERROR_BASE");
    error_register(PARSEB16_ERROR, "PARSEB16_ERROR");
    error_register(PARSEB32_ERROR, "PARSEB32_ERROR");
    error_register(PARSEB32H_ERROR, "PARSEB32H_ERROR");
    error_register(PARSEB64_ERROR, "PARSEB64_ERROR");
    error_register(PARSEINT_ERROR, "PARSEINT_ERROR");
    error_register(PARSEDATE_ERROR, "PARSEDATE_ERROR");
    error_register(PARSEIP_ERROR, "PARSEIP_ERROR");
    
    error_register(TCP_RATE_TOO_SLOW, "TCP_RATE_TOO_SLOW");

    error_register(PARSEWORD_NOMATCH_ERROR, "PARSEWORD_NOMATCH_ERROR");
    error_register(PARSESTRING_ERROR, "PARSESTRING_ERROR");
    error_register(PARSE_BUFFER_TOO_SMALL_ERROR, "PARSE_BUFFER_TOO_SMALL_ERROR");
    error_register(PARSE_INVALID_CHARACTER, "PARSE_INVALID_CHARACTER");

    error_register(LOGGER_INITIALISATION_ERROR, "LOGGER_INITIALISATION_ERROR");
    error_register(COMMAND_ARGUMENT_EXPECTED, "COMMAND_ARGUMENT_EXPECTED");
    error_register(OBJECT_NOT_INITIALIZED, "OBJECT_NOT_INITIALIZED");
    error_register(FORMAT_ALREADY_REGISTERED, "FORMAT_ALREADY_REGISTERED");
    error_register(STOPPED_BY_APPLICATION_SHUTDOWN, "STOPPED_BY_APPLICATION_SHUTDOWN");

    error_register(UNABLE_TO_COMPLETE_FULL_READ, "UNABLE_TO_COMPLETE_FULL_READ");
    error_register(UNEXPECTED_EOF, "UNEXPECTED_EOF");
    error_register(UNSUPPORTED_TYPE, "UNSUPPORTED_TYPE");
    error_register(UNKNOWN_NAME, "UNKNOWN_NAME");
    error_register(BIGGER_THAN_MAX_PATH, "BIGGER_THAN_MAX_PATH");

    error_register(THREAD_CREATION_ERROR, "THREAD_CREATION_ERROR");
    error_register(THREAD_DOUBLEDESTRUCTION_ERROR, "THREAD_DOUBLEDESTRUCTION_ERROR");

    error_register(TSIG_DUPLICATE_REGISTRATION, "TSIG_DUPLICATE_REGISTRATION");
    error_register(TSIG_UNABLE_TO_SIGN, "TSIG_UNABLE_TO_SIGN");

    error_register(NET_UNABLE_TO_RESOLVE_HOST, "NET_UNABLE_TO_RESOLVE_HOST");

    error_register(ALARM_REARM, "ALARM_REARM");

    error_register(DNS_ERROR_BASE, "DNS_ERROR_BASE");
    error_register(DOMAIN_TOO_LONG, "DOMAIN_TOO_LONG");
    error_register(INCORRECT_IPADDRESS, "INCORRECT_IPADDRESS");
    error_register(INCORRECT_RDATA, "INCORRECT_RDATA");
    error_register(SALT_TO_BIG, "SALT_TO_BIG");
    error_register(SALT_NOT_EVEN_LENGTH, "SALT_NOT_EVEN_LENGTH");
    error_register(HASH_TOO_BIG, "HASH_TOO_BIG");
    error_register(SALT_NOT_EVEN_LENGTH, "SALT_NOT_EVEN_LENGTH");
    error_register(HASH_TOO_BIG, "HASH_TOO_BIG");
    error_register(HASH_NOT_X8_LENGTH, "HASH_NOT_X8_LENGTH");
    error_register(HASH_TOO_SMALL, "HASH_TOO_SMALL");
    error_register(HASH_BASE32DECODE_FAILED, "HASH_BASE32DECODE_FAILED");
    error_register(HASH_BASE32DECODE_WRONGSIZE, "HASH_BASE32DECODE_WRONGSIZE");
    error_register(ZONEFILE_UNSUPPORTED_TYPE, "ZONEFILE_UNSUPPORTED_TYPE");
    error_register(LABEL_TOO_LONG, "LABEL_TOO_LONG");
    error_register(INVALID_CHARSET, "INVALID_CHARSET");
   
    error_register(NO_LABEL_FOUND, "NO_LABEL_FOUND");
    error_register(NO_ORIGIN_FOUND, "NO_ORIGIN_FOUND");
    error_register(DOMAINNAME_INVALID, "DOMAINNAME_INVALID");
    error_register(TSIG_BADKEY, "TSIG_BADKEY");
    error_register(TSIG_BADTIME, "TSIG_BADTIME");
    error_register(TSIG_BADSIG, "TSIG_BADSIG");
    error_register(TSIG_FORMERR, "TSIG_FORMERR");
    error_register(TSIG_SIZE_LIMIT_ERROR, "TSIG_SIZE_LIMIT_ERROR");
    error_register(UNPROCESSABLE_MESSAGE, "UNPROCESSABLE_MESSAGE");

    error_register(INVALID_PROTOCOL, "INVALID_PROTOCOL");
    error_register(INVALID_MESSAGE, "INVALID_MESSAGE");
    error_register(MESSAGE_HAS_WRONG_ID, "MESSAGE_HAS_WRONG_ID");
    error_register(MESSAGE_IS_NOT_AN_ANSWER, "MESSAGE_IS_NOT_AN_ANSWER");
    error_register(MESSAGE_UNEXCPECTED_ANSWER_DOMAIN, "MESSAGE_UNEXCPECTED_ANSWER_DOMAIN");
    error_register(MESSAGE_UNEXCPECTED_ANSWER_TYPE_CLASS, "MESSAGE_UNEXCPECTED_ANSWER_TYPE_CLASS");

    /* DNS */

    error_register(MAKE_DNSMSG_ERROR(RCODE_NOERROR), "DNSMSG_ERROR_RCODE_NOERROR");
    error_register(MAKE_DNSMSG_ERROR(RCODE_FORMERR), "DNSMSG_ERROR_RCODE_FORMERR");
    error_register(MAKE_DNSMSG_ERROR(RCODE_SERVFAIL), "DNSMSG_ERROR_RCODE_SERVFAIL");
    error_register(MAKE_DNSMSG_ERROR(RCODE_NXDOMAIN), "DNSMSG_ERROR_RCODE_NXDOMAIN");
    error_register(MAKE_DNSMSG_ERROR(RCODE_NOTIMP), "DNSMSG_ERROR_RCODE_NOTIMP");
    error_register(MAKE_DNSMSG_ERROR(RCODE_REFUSED), "DNSMSG_ERROR_RCODE_REFUSED");
    error_register(MAKE_DNSMSG_ERROR(RCODE_YXDOMAIN), "DNSMSG_ERROR_RCODE_YXDOMAIN");
    error_register(MAKE_DNSMSG_ERROR(RCODE_YXRRSET), "DNSMSG_ERROR_RCODE_YXRRSET");
    error_register(MAKE_DNSMSG_ERROR(RCODE_NXRRSET), "DNSMSG_ERROR_RCODE_NXRRSET");
    error_register(MAKE_DNSMSG_ERROR(RCODE_NOTAUTH), "DNSMSG_ERROR_RCODE_NOTAUTH");
    error_register(MAKE_DNSMSG_ERROR(RCODE_NOTZONE), "DNSMSG_ERROR_RCODE_NOTZONE");
}

/** @} */

/*----------------------------------------------------------------------------*/

