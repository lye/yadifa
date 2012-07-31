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
 *  @brief Functions used to manipulate dns formatted names and labels
 *
 * DNS names are stored in many ways:
 * _ C string : ASCII with a '\0' sentinel
 * _ DNS wire : label_length_byte + label_bytes) ending with a label_length_byte with a value of 0
 * _ simple array of pointers to labels
 * _ simple stack of pointers to labels (so the same as above, but with the order reversed)
 * _ sized array of pointers to labels
 * _ sized stack of pointers to labels (so the same as above, but with the order reversed)
 * 
 * @{
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "dnscore/dnsname.h"
#include "dnscore/rfc.h"

#define DNSNAMED_TAG 0x44454d414e534e44

/*****************************************************************************
 *
 * BUFFER
 *
 *****************************************************************************/

/** @brief Converts a C string to a dns name.
 *
 *  Converts a C string to a dns name.
 *
 *  @param[in] str a pointer to the source c-string
 *  @param[in] name a pointer to a buffer that will get the full dns name
 *
 *  @return Returns the length of the string
 */

/* TWO uses */

/*
 * This table contains TRUE for both expected name terminators
 */

static bool cstr_to_dnsname_terminators[256] =
{
    TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, /* '\0' */
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, /* '.' */
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
};

/*
 * This table contains TRUE for each character in the DNS charset
 */

static bool cstr_to_dnsname_charspace[256] =
{
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, /* 00 */
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, /* 10 */
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, /* 20 */
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, /* 30 */
    FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  /* 40 */
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, /* 50 */
    FALSE, TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  /* 60 */
    TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, /* 70 */
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
    FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
};

/**
 *  0: out of space
 *  1: in space
 * -1: terminator
 *
 * =>
 *
 * test the map
 * signed -> terminator
 *   zero -> out of space
 *
 */
static s8 cstr_to_dnsname_map[256] =
{
   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 (HEX) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1,-1, 0, /* 20 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 50 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static s8 cstr_to_dnsname_map_nostar[256] =
{
   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 (HEX) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,-1, 0, /* 20 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 50 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**
 * This is a set for rname in the SOA TYPE
 *
 *  0: out of space
 *  1: in space
 * -1: terminator
 *
 * =>
 *
 * test the map
 * signed -> terminator
 *   zero -> out of space
 *
 */

static s8 cstr_to_dnsrname_map[256] =
{
   -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00 (HEX) */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,-1, 0, /* 20 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 2, 0, 0, 1, /* 50 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/**
 * char DNS charset test
 * 
 * @param c
 * @return TRUE iff c in in the DNS charset
 * 
 */

bool
dnsname_is_charspace(u8 c)
{
    return cstr_to_dnsname_map[c] == 1;
}

/**
 * label DNS charset test
 * 
 * @param label
 * @return TRUE iff each char in the label in in the DNS charset
 * 
 */

bool
dnslabel_verify_charspace(u8 *label)
{
    u8 n = *label;    

    if(n > MAX_LABEL_LENGTH)
    {
        return FALSE;
    }

    u8 *limit = &label[n];

    while(++label < limit)
    {
        u8 c = *label;

        if(cstr_to_dnsname_map[c] != 1)
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * label DNS charset test and set to lower case
 * 
 * @param label
 * @return TRUE iff each char in the label in in the DNS charset
 * 
 */

bool
dnslabel_locase_verify_charspace(u8 *label)
{
    u8 n = *label;

    if(n > MAX_LABEL_LENGTH)
    {
        return FALSE;
    }

    u8 *limit = &label[n];

    while(++label <= limit)
    {
        u8 c = *label;

        if(cstr_to_dnsname_map[c] != 1)
        {
            return FALSE;
        }

        *label = LOCASE(c);
    }

    return TRUE;
}

/**
 * dns name DNS charset test and set to lower case
 * 
 * LOCASE is done using |32
 * 
 * @param name_wire
 * @return TRUE iff each char in the name in in the DNS charset
 * 
 */

bool
dnsname_locase_verify_charspace(u8 *name_wire)
{
    u8 n;
    
    for(;;)
    {
        n = *name_wire;
        
        if(n == 0)
        {
            return TRUE;
        }

        if(n > MAX_LABEL_LENGTH)
        {
            return FALSE;
        }

        u8 *limit = &name_wire[n];

        while(++name_wire <= limit)
        {
            u8 c = *name_wire;

            if(cstr_to_dnsname_map[c] != 1)
            {
                return FALSE;
            }

            *name_wire = LOCASE(c);
        }
    }
}

/**
 * dns name DNS charset test and set to lower case
 * 
 * LOCASE is done using tolower(c)
 * 
 * @param name_wire
 * @return TRUE iff each char in the name in in the DNS charset
 * 
 */

bool
dnsname_locase_verify_extended_charspace(u8 *name_wire)
{
    u8 n;
    
    for(;;)
    {
        n = *name_wire;
        
        if(n == 0)
        {
            return TRUE;
        }

        if(n > MAX_LABEL_LENGTH)
        {
            return FALSE;
        }

        u8 *limit = &name_wire[n];

        while(++name_wire <= limit)
        {
            u8 c = *name_wire;

            if(cstr_to_dnsname_map[c] != 1)
            {
                return FALSE;
            }

            *name_wire = tolower(c);
        }
    }
}

/**
 *  @brief Converts a C string to a dns name.
 *
 *  Converts a C string to a dns name.
 *
 *  @param[in] name_parm a pointer to a buffer that will get the full dns name
 *  @param[in] str a pointer to the source c-string
 *
 *  @return Returns the length of the string up to the last '\0'
 */

ya_result
cstr_to_dnsname(u8* name_parm, const char* str)
{
    u8 *limit = &name_parm[MAX_DOMAIN_LENGTH];
    u8 *s = name_parm;
    u8 *p = &name_parm[1];

    u8 c;

    for(c = *str++;; c = *str++)
    {
        if(!cstr_to_dnsname_terminators[c] /*(c != '.') && (c != '\0')*/)
        {
            *p = c;
        }
        else
        {
            u8 l = p - s - 1;
            *s = l;
            s = p;

            if(l == 0)
            {
                break;
            }

            if(l > MAX_LABEL_LENGTH)
            {
                return LABEL_TOO_LONG;
            }

            if(c == '\0')
            {
                if(p >= limit)
                {
                    return DOMAIN_TOO_LONG;
                }

                *s++ = '\0';
                break;
            }
        }

        if(++p > limit)
        {
            return DOMAIN_TOO_LONG;
        }
    }

    return s - name_parm;
}

/**
 *  @brief Converts a C string to a dns name and checks for validity
 *
 *  Converts a C string to a dns name.
 *
 *  @param[in] name_parm a pointer to a buffer that will get the full dns name
 *  @param[in] str a pointer to the source c-string
 *
 *  @return Returns the length of the string up to the last '\0'
 */

ya_result
cstr_to_dnsname_with_check(u8* name_parm, const char* str)
{
    u8 *limit = &name_parm[MAX_DOMAIN_LENGTH];
    u8 *s = name_parm;
    u8 *p = &name_parm[1];

    u8 c;

    /*
     * I cannot check this in one go actually.
     *
     * It would work 99.9999999% of the time but if the string is "" and is at the end of a page it would overlap to a non-mapped
     * memory and crash.
     *
     */

    if((str[0] == '.') && (str[1] == '\0'))
    {
        *name_parm = 0;
        return 1;
    }

    if(str[0] == '*')
    {
        if(str[1] == '\0')
        {
            name_parm[0] = 1;
            name_parm[1] = '*';
            name_parm[2] = '\0';
            return 3;
        }
        else if(str[1] == '.')
        {
            name_parm[0] = 1;
            name_parm[1] = '*';
            str += 2;
            s += 2;
            p += 2;
        }
        else
        {
            return DOMAINNAME_INVALID;
        }
    }

    for(c = *str++;; c = *str++)
    { /* test if a switch/case is better (break mix issues for this switch in this particular loop)
	   *
	   * in theory this is test/jb/jz
	   * a switch would be jmp [v]
	   *
	   */
        
        if(cstr_to_dnsname_map_nostar[c] >= 0 /*(c != '.') && (c != '\0')*/)
        {
            if(cstr_to_dnsname_map_nostar[c] == 0)
            {
                return INVALID_CHARSET;
            }

            *p = c;
        }
        else
        {
            u8 l = p - s - 1;
            *s = l;
            s = p;

            if(l == 0)
            {
                if(c != '\0')
                {
                    return DOMAINNAME_INVALID;
                }

                break;
            }

            if(l > MAX_LABEL_LENGTH)
            {
                return LABEL_TOO_LONG;
            }

            if(c == '\0')
            {
                if(p >= limit)
                {
                    return DOMAIN_TOO_LONG;
                }

                *s++ = '\0';
                break;
            }
        }

        if(++p > limit)
        {
            return DOMAIN_TOO_LONG;
        }
    }

    return s - name_parm;
}

/**
 *  @brief Converts a C string to a dns rname and checks for validity
 *
 *  Converts a C string to a dns rname.
 *
 *  @param[in] name_parm a pointer to a buffer that will get the full dns name
 *  @param[in] str a pointer to the source c-string
 *
 *  @return the length of the string up to the last '\0'
 */

ya_result
cstr_to_dnsrname_with_check(u8* name_parm, const char* str)
{
    u8 *limit = &name_parm[MAX_DOMAIN_LENGTH];
    u8 *s = name_parm;
    u8 *p = &name_parm[1];

    u8 c;

    bool escaped = FALSE;

    /*
     * I cannot check this in one go actually.
     *
     * It would work 99.9999999% of the time but if the string is "" and is at the end of a page it would overlap to a non-mapped
     * memory and crash.
     *
     */

    for(c = *str++;; c = *str++)
    { /* test if a switch/case is better (break mix issues for this switch in this particular loop)
	 *
	 * in theory this is test/jb/jz
	 * a switch would be jmp [v]
	 * mmhh ...
	 *
	 */
        if((cstr_to_dnsrname_map[c] >= 0)  /*(c != '.') && (c != '\0')*/ || escaped)
        {
            if(!escaped)
            {            
                if(cstr_to_dnsrname_map[c] == 0)
                {
                    return INVALID_CHARSET;
                }

                /* escape character */
                /* AFAIK there is only one escape : '\', so why use an indexed memory to test it ?  if (cstr_to_dnsrname_map[c] == 2) */
                if(c == '\\')
                {  
                    escaped = TRUE;
                    /* reading the next char here is wrong is the record is corrupt and ends with my-wrong-rname\. */
                    continue;
                }
            }

            *p = c;
            escaped = FALSE;
        }
        else
        {
            u8 l = p - s - 1;
            *s = l;

            s = p;

            if(l == 0)
            {
                if(c != '\0')
                {
                    return DOMAINNAME_INVALID;
                }

                break;
            }

            if(l > MAX_LABEL_LENGTH)
            {
                return LABEL_TOO_LONG;
            }

            if(c == '\0')
            {
                if(p >= limit)
                {
                    return DOMAIN_TOO_LONG;
                }

                *s++ = '\0';
                break;
            }
        }

        if(++p > limit)
        {
            return DOMAIN_TOO_LONG;
        }
    }
    
    if(escaped)
    {
        return DOMAINNAME_INVALID;
    }

    return s - name_parm;
}

/* ONE use */

ya_result
cstr_get_dnsname_len(const char* str)
{
    ya_result total = 0;
    const char* start;

    if(*str == '.')
    {
        str++;
    }

    s32 label_len;

    do
    {
        char c;

        start = str;

        do
        {
            c = *str++;
        }
        while(c != '.' && c != '\0');

        label_len = (str - start) - 1;

        if(label_len > MAX_LABEL_LENGTH)
        {
            return LABEL_TOO_LONG;
        }

        total += label_len + 1;

        if(c == '\0')
        {
            if(label_len != 0)
            {
                total++;
            }

            break;
        }
    }
    while(label_len != 0);

    return total;
}

/** @brief Converts a dns name to a C string
 *
 *  Converts a dns name to a C string
 *
 *  @param[in] name a pointer to the source dns name
 *  @param[in] str a pointer to a buffer that will get the c-string
 *
 *  @return Returns the length of the string
 */

/* FOUR uses */

u32
dnsname_to_cstr(char* dest_cstr, const u8* name)
{
#if defined(DEBUG)
    zassert(name != NULL);
#endif
    
    char* start = dest_cstr;

    u8 len;

    len = *name++;

    if(len != 0)
    {
        do
        {
            MEMCOPY(dest_cstr, name, len);
            dest_cstr += len;
            *dest_cstr++ = '.';
            name += len;
            len = *name++;
        }
        while(len != 0);
    }
    else
    {
        *dest_cstr++ = '.';
    }

    *dest_cstr++ = '\0';

    return (u32)(dest_cstr - start);
}

/** @brief Tests if two DNS labels are equals
 *
 *  Tests if two DNS labels are equals
 *
 *  @param[in] name_a a pointer to a dnsname to compare
 *  @param[in] name_b a pointer to a dnsname to compare
 *
 *  @return Returns TRUE if names are equal, else FALSE.
 */

/* ELEVEN uses */

#if HAS_MEMALIGN_ISSUES == 0

bool
dnslabel_equals(const u8* name_a, const u8* name_b)
{
    u8 len = *name_a;

    if(len != *name_b)
    {
        return FALSE;
    }

    len++;

    /* Hopefully the compiler just does register renaming */

    const u32* name_a_32 = (const u32*)name_a;
    const u32* name_b_32 = (const u32*)name_b;

    while(len >= 4)
    {
        if(*name_a_32 != *name_b_32)
        {
            return FALSE;
        }
        
        name_a_32++;
        name_b_32++;

        len -= 4;
    }

    /* Hopefully the compiler just does register renaming */

    name_a = (const u8*)name_a_32;
    name_b = (const u8*)name_b_32;

    switch(len)
    {
        case 0:
            return TRUE;
        case 1:
            return *name_a == *name_b;
        case 2:
            return GET_U16_AT(*name_a) == GET_U16_AT(*name_b);
        case 3:
            return (GET_U16_AT(*name_a) == GET_U16_AT(*name_b)) && (name_a[2] == name_b[2]);
    }

    // icc complains here but is wrong.
    // this line cannot EVER be reached
    
    assert(FALSE); /* NOT zassert */
    
    return FALSE;
}

#else

bool
dnslabel_equals(const u8* name_a, const u8* name_b)
{
    u8 len = *name_a;

    if(*name_b == len)
    {
        return memcmp(name_a + 1, name_b + 1, len) == 0;
    }

    return FALSE;
}

#endif

/** @brief Tests if two DNS labels are (case-insensitive) equals
 *
 *  Tests if two DNS labels are (case-insensitive) equals
 *
 *  @param[in] name_a a pointer to a dnsname to compare
 *  @param[in] name_b a pointer to a dnsname to compare
 *
 *  @return Returns TRUE if names are equal, else FALSE.
 */

#if HAS_MEMALIGN_ISSUES == 0

bool
dnslabel_equals_ignorecase_left(const u8* name_a, const u8* name_b)
{
    int len = (int)* name_a;

    if(len != (int)* name_b)
    {
        return FALSE;
    }

    len++;

    /* Hopefully the compiler just does register renaming */

    const u32* name_a_32 = (const u32*)name_a;
    const u32* name_b_32 = (const u32*)name_b;

    /*
     * Label size must match
     */

    while(len >= 4)
    {
        if(((*name_a_32++ - *name_b_32++) & 0xdfdfdfdf) != 0)   /* ignore case */
        {
            return FALSE;
        }

        len -= 4;
    }

    /* Hopefully the compiler just does register renaming */

    name_a = (const u8*)name_a_32;
    name_b = (const u8*)name_b_32;

    switch(len)
    {
        case 0:
            return TRUE;
        case 1:
            return LOCASEEQUALS(*name_a, *name_b);
        case 2:
            return (((GET_U16_AT(*name_a) - GET_U16_AT(*name_b)) & ((u16)0xdfdf)) == 0);
        case 3:
            return (((GET_U16_AT(*name_a) - GET_U16_AT(*name_b)) & ((u16)0xdfdf)) == 0) && LOCASEEQUALS(name_a[2], name_b[2]);
    }

    assert(FALSE); /* NOT zassert */
    
    return FALSE;
}

#else

/**
 * This WILL work with label size too since a label size is 0->63
 * which is well outside the [A-Za-z] space.
 */

bool
dnslabel_equals_ignorecase_left(const u8* name_a, const u8* name_b)
{
    return strcasecmp((const char*)name_a, (const char*)name_b) == 0;
}

#endif

/** @brief Tests if two DNS names are equals
 *
 *  Tests if two DNS labels are equals
 *
 *  @param[in] name_a a pointer to a dnsname to compare
 *  @param[in] name_b a pointer to a dnsname to compare
 *
 *  @return Returns TRUE if names are equal, else FALSE.
 */

/* TWO uses */

bool
dnsname_equals(const u8* name_a, const u8* name_b)
{
    int la = dnsname_len(name_a);
    int lb = dnsname_len(name_b);

    if(la == lb)
    {
        return memcmp(name_a, name_b, la) == 0;
    }

    return FALSE;
}

/*
 * Comparison of a name by label
 */

int
dnsname_compare(const u8* name_a, const u8* name_b)
{
    for(;;)
    {
        s8 la = (s8)name_a[0];
        s8 lb = (s8)name_b[0];
        
        name_a++;
        name_b++;
        
        if(la == lb)
        {   
            if( la > 0)
            {
                int c =  memcmp(name_a, name_b, la);

                if( c != 0)
                {
                    return c;
                }
            }
            else
            {
                return 0;
            }
        }
        else
        {   
            int c =  memcmp(name_a, name_b, MIN(la,lb));
            
            if( c == 0)
            {
                c = la - lb;
            }
            
            return c;
        }
        
        name_a += la;
        name_b += lb;
    }
}

int
dnsname_compare_broken(const u8* name_a, const u8* name_b)
{
    const u8* start;
    const u8* name;

    u8 c;

    int da = 0;
    int db = 0;

    start = name_a;
    name = name_a;

    while((c = *name++) > 0)
    {
        name += c;
        da++;
    }

    int la = name - start;

    start = name_b;
    name = name_b;

    while((c = *name++) > 0)
    {
        name += c;
        db++;
    }

    int lb = name - start;

    if(da != db)
    {
        /* the one with less labels comes first */  /** @wtf ??? */

        return da - db;
    }

    while(da > 0)
    {
        u8 na = *name_a++;
        u8 nb = *name_b++;

        s32 diff;

        if(na != nb)
        {
            u8 n = MIN(na, nb);

            diff = memcmp(name_a, name_b, n);

            if(diff == 0)
            {
                diff = ((s8)na - (s8)nb);
            }

            return diff;
        }
        else
        {
            if((diff = memcmp(name_a, name_b, na)) != 0)
            {
                return diff;
            }

            name_a += na;
            name_b += na;
        }
        
        da--;
    }

    return 0;
}

/** @brief Tests if two DNS names are (ignore case) equals
 *
 *  Tests if two DNS labels are (ignore case) equals
 *
 *  @param[in] name_a a pointer to a dnsname to compare
 *  @param[in] name_b a pointer to a dnsname to compare
 *
 *  @return Returns TRUE if names are equal, else FALSE.
 */

/* TWO uses */

bool
dnsname_equals_ignorecase(const u8* name_a, const u8* name_b)
{
    int len;

    do
    {
        len = (int)*name_a++;

        if(len != (int)*name_b++)
        {
            return FALSE;
        }

        if(len == 0)
        {
            return TRUE;
        }

        while(len > 0 && (LOCASE(*name_a++) == LOCASE(*name_b++))) len--;
    }
    while(len == 0);

    return FALSE;
}

/** @brief Returns the full length of a dns name
 *
 *  Returns the full length of a dns name
 *
 *  @param[in] name a pointer to the dnsname
 *
 *  @return The length of the dnsname, "." ( zero ) included
 */

/* SEVENTEEN uses (more or less) */

u32
dnsname_len(const u8 *name)
{
    zassert(name != NULL);
    
    const u8* start = name;

    u8 c;

    while((c = *name++) > 0)
    {
        name += c;
    }

    return name - start;
}

/* ONE use */

u32
dnsname_getdepth(const u8 *name)
{
    zassert(name != NULL);
    
    u32 d = 0;

    u8 c;

    while((c = *name++) > 0)
    {
        name += c;
        d++;
    }

    return d;
}

u8*
dnsname_dup(const u8* src)
{
    u8 *dst;
    u32 len = dnsname_len(src);
    MALLOC_OR_DIE(u8*, dst, len, DNSNAMED_TAG);
    MEMCOPY(dst, src, len);

    return dst;
}

/* ONE use */

u32
dnsname_copy(u8* dst, const u8* src)
{
    u32 len = dnsname_len(src);

    MEMCOPY(dst, src, len);

    return len;
}

/** @brief Canonizes a dns name.
 *
 *  Canonizes a dns name. (A.K.A : lo-cases it)
 *
 *  @param[in] src a pointer to the dns name
 *  @param[out] dst a pointer to a buffer that will hold the canonized dns name
 *
 *  @return The length of the dns name
 */

/* TWELVE uses */

u32
dnsname_canonize(const u8* src, u8* dst)
{
    const u8* org = src;

    u32 len;

    for(;;)
    {
        len = *src++;
        *dst++ = len;

        if(len == 0)
        {
            break;
        }

        while(len > 0)
        {
            *dst++ = LOCASE(*src++); /* Works with the dns character set */
            len--;
        }
    }

    return (u32)(src - org);
}

/*****************************************************************************
 *
 * VECTOR
 *
 *****************************************************************************/

/* NO use (test) */

u32
dnslabel_vector_to_dnsname(const_dnslabel_vector_reference name, s32 top, u8* str_start)
{
    u8* str = str_start;

    const_dnslabel_vector_reference limit = &name[top];

    while(name <= limit)
    {
        u8* label = *name++;
        u8 len = label[0] + 1;
        MEMCOPY(str, label, len);
        str += len;
    }

    *str++ = 0;

    return str - str_start;
}

/* NO use (test) */

u32
dnslabel_vector_to_cstr(const_dnslabel_vector_reference name, s32 top, char* str)
{
    const_dnslabel_vector_reference limit = &name[top];

    char* start = str;

    while(name < limit)
    {
        u8* label = *name++;
        u8 len = *label++;

        MEMCOPY(str, label, len);
        str += len;

        *str++ = '.';
    }

    *str++ = '\0';

    return (u32)(str - start);
}

/* ONE use */

u32
dnslabel_vector_dnslabel_to_dnsname(const u8 *prefix, const dnsname_vector *namestack, s32 bottom, u8 *str)
{
    u8* start = str;

    u32 len = *prefix;
    MEMCOPY(str, prefix, len + 1);
    str += len + 1;

    const_dnslabel_vector_reference name = &namestack->labels[bottom];
    u32 top = namestack->size;

    while(bottom <= top)
    {
        u8* label = *name++;
        u32 len = *label;

        MEMCOPY(str, label, len + 1);
        str += len + 1;

        bottom--;
    }

    *str++ = '\0';

    return (u32)(str - start);
}

/* ONE use */

u32
dnsname_vector_sub_to_dnsname(const dnsname_vector *name, s32 from, u8 *name_start)
{
    u8* str = name_start;

    const_dnslabel_vector_reference limit = &name->labels[name->size];
    const_dnslabel_vector_reference labelp = &name->labels[from];

    while(labelp <= limit)
    {
        u32 len = *labelp[0] + 1;
        MEMCOPY(str, *labelp, len);
        str += len;
        labelp++;
    }

    *str++ = 0;

    return str - name_start;
}

/** @brief Divides a name into sections
 *
 *  Divides a name into sections.
 *  Writes a pointer to each label of the dnsname into an array
 *  "." is never put in there.
 *
 *  @param[in] name a pointer to the dnsname
 *  @param[out] sections a pointer to the target array of pointers
 *
 *  @return The index of the top-level label ("." is never put in there)
 */

/* TWO uses */

s32
dnsname_to_dnslabel_vector(const u8 *dns_name, dnslabel_vector_reference labels)
{
    zassert(dns_name != NULL && labels != NULL);
    
    s32 idx = -1;
    int offset = 0;

    for(;;)
    {
        u32 len = dns_name[offset];

        if(len == 0)
        {
            break;
        }

        labels[++idx] = (u8*) & dns_name[offset];
        offset += len + 1;
    }

    return idx;
}

/** @brief Divides a name into sections
 *
 *  Divides a name into sections.
 *  Writes a pointer to each label of the dnsname into an array
 *  "." is never put in there.
 *
 *  @param[in] name a pointer to the dnsname
 *  @param[out] sections a pointer to the target array of pointers
 *
 *  @return The index of the top-level label ("." is never put in there)
 */

/* TWENTY-ONE uses */

s32
dnsname_to_dnsname_vector(const u8* dns_name, dnsname_vector* name)
{
    zassert(dns_name != NULL && name != NULL);
    
    s32 size = dnsname_to_dnslabel_vector(dns_name, name->labels);
    name->size = size;

    return size;
}

u32 dnsname_vector_copy(dnsname_vector* dst, const dnsname_vector* src)
{
    dst->size = src->size;
    if(dst->size > 0)
    {
        memcpy(&dst->labels[0], &src->labels[0], sizeof(u8*) * dst->size);
    }
    return dst->size;
}

/*****************************************************************************
 *
 * STACK
 *
 *****************************************************************************/

/** @brief Converts a stack of dns labels to a C string
 *
 *  Converts a stack of dns labels to a C string
 *
 *  @param[in] name a pointer to the dnslabel stack
 *  @param[in] top the index of the top of the stack
 *  @param[in] str a pointer to a buffer that will get the c-string
 *
 *  @return Returns the length of the string
 */

/* ONE use */

u32
dnslabel_stack_to_cstr(const_dnslabel_stack_reference name, s32 top, char* str)
{
    char* start = str;
    while(top >= 0)
    {
        u8* label = name[top];
        u8 len = *label++;

        MEMCOPY(str, label, len);
        str += len;

        *str++ = '.';
        top--;
    }
    *str++ = '\0';

    return (u32)(str - start);
}

/* ONE use */

u32
dnslabel_stack_to_dnsname(const_dnslabel_stack_reference name, s32 top, u8* str_start)
{

    u8* str = str_start;
    const_dnslabel_stack_reference base = name;

    name += top;

    while(name >= base)
    {
        u8* label = *name--;
        u32 len = *label;

        MEMCOPY(str, label, len + 1);
        str += len + 1;
    }

    *str++ = '\0';

    return (u32)(str - str_start);
}

s32
dnsname_to_dnslabel_stack(const u8* dns_name, dnslabel_stack_reference labels)
{
    s32 label_pointers_top = -1;
    const u8 * label_pointers[MAX_LABEL_COUNT];

    for(;;)
    {
        u8 len = *dns_name;

        if(len == 0)
        {
            break;
        }

        label_pointers[++label_pointers_top] = dns_name;

        dns_name += len + 1;
    }

    s32 size = label_pointers_top;

    u8** labelp = labels;
    while(label_pointers_top >= 0)
    {
        *labelp++ = (u8*)label_pointers[label_pointers_top--];
    }

    return size;
}

/* ONE use */

u32
dnsname_stack_to_dnsname(const dnsname_stack* name_stack, u8* name_start)
{
    u8* name = name_start;

    s32 size = name_stack->size;

    for(; size >= 0; size--)
    {
        u32 len = name_stack->labels[size][0] + 1;
        MEMCOPY(name, name_stack->labels[size], len);
        name += len;
    }

    *name++ = '\0';

    return name - name_start;
}

/* TWO uses (debug) */

u32
dnsname_stack_to_cstr(const dnsname_stack* name, char* str)
{
    return dnslabel_stack_to_cstr(name->labels, name->size, str);
}

/* ONE use */

bool
dnsname_equals_dnsname_stack(const u8* str, const dnsname_stack* name)
{
    s32 size = name->size;

    while(size >= 0)
    {
        u8* label = name->labels[size];
        u8 len = *label;

        if(len != *str)
        {
            return FALSE;
        }

        label++;
        str++;

        if(memcmp(str, label, len) != 0)
        {
            return FALSE;
        }

        str += len;

        size--;
    }

    return *str == 0;
}

bool
dnsname_under_dnsname_stack(const u8* str, const dnsname_stack* name)
{
    s32 size = name->size;

    while(size >= 0)
    {
        u8* label = name->labels[size];
        u8 len = *label;

        if(len != *str)
        {
            return (len != 0);
        }

        label++;
        str++;

        if(memcmp(str, label, len) != 0)
        {
            return FALSE;
        }

        str += len;

        size--;
    }

    return *str == 0;
}

/* FOUR uses */

s32
dnsname_stack_push_label(dnsname_stack* dns_name, u8* dns_label)
{
    zassert(dns_name != NULL && dns_label != NULL);
   
    dns_name->labels[++dns_name->size] = dns_label;

    return dns_name->size;
}

/* FOUR uses */

s32
dnsname_stack_pop_label(dnsname_stack* name)
{
    zassert(name != NULL);
    
#ifndef NDEBUG
    name->labels[name->size] = (u8*)~0;
#endif

    return name->size--;
}

s32
dnsname_to_dnsname_stack(const u8* dns_name, dnsname_stack* name)
{
    s32 label_pointers_top = -1;
    u8 * label_pointers[MAX_LABEL_COUNT];

    for(;;)
    {
        u8 len = *dns_name;

        if(len == 0)
        {
            break;
        }

        label_pointers[++label_pointers_top] = (u8*)dns_name;

        dns_name += len + 1;
    }

    name->size = label_pointers_top;

    u8** labelp = name->labels;
    while(label_pointers_top >= 0)
    {
        *labelp++ = label_pointers[label_pointers_top--];
    }

    return name->size;
}

/** @} */
