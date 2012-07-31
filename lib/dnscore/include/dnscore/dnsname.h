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
#ifndef _DNSNAME_H
#define	_DNSNAME_H

#include <dnscore/sys_types.h>
#include <dnscore/rfc.h>
/**
 * The maximum number of domains-subdomains handled by the database.
 * This should not be set to a value greater than 128 (128 covers (\001 'a') * 255 )
 *
 * Recommended value: 128
 *
 */

#define DNSNAME_MAX_SECTIONS ((MAX_DOMAIN_LENGTH+1) / 2)

#define LOCASE(c__) ((char)(c__)|(char)0x20)
#define LOCASEEQUALS(ca__,cb__) ((((char)(ca__)-(char)(cb__))&0xdf) == 0)

#define ZDB_NAME_TAG  0x454d414e42445a       /* "ZDBNAME" */
#define ZDB_LABEL_TAG 0x4c424c42445a         /* "ZDBLBL" */

#ifdef	__cplusplus
extern "C"
{
#endif

/*
 * A dnslabel_array is basically a dnslabel*[]
 * There are two kind of arrays :
 *
 * dnslabel_stack:
 *
 * [0000] "."	    label
 * [0001] "tdl"	    label
 * [0002] "domain"  label <- top
 *
 * dnslabel_vector:
 *
 * [0000] "domain"  label
 * [0001] "tdl"	    label
 * [0002] "."	    label <- top
 *
 */

/*
 * The plan was to typedef an array into a stack or a vector.
 * But in order to help the compiler complaining about mixing both,
 * I have to define them separatly
 *
 */

typedef u8* dnslabel_stack[DNSNAME_MAX_SECTIONS];

/* This + 1 is just to make sure both are different to the compiler's eyes */

typedef u8* dnslabel_vector[DNSNAME_MAX_SECTIONS + 1];

typedef u8** dnslabel_stack_reference;
typedef u8** dnslabel_vector_reference;

typedef u8*const* const_dnslabel_stack_reference;
typedef u8*const* const_dnslabel_vector_reference;

#ifndef NDEBUG
#define DEBUG_RESET_dnsname(name) memset(&(name),0x5b,sizeof(dnsname_stack))
#else
#define DEBUG_RESET_dnsname(name)
#endif

typedef struct dnsname_stack dnsname_stack;


struct dnsname_stack
{
    s32 size;
    dnslabel_stack labels;
};

typedef struct dnsname_vector dnsname_vector;


struct dnsname_vector
{
    s32 size;
    dnslabel_vector labels;
};

/*****************************************************************************
 *
 * BUFFER
 *
 *****************************************************************************/

/** @brief Converts a C string to a dns name.
 *
 *  Converts a C string to a dns name.
 *
 *  @param[in] name_parm a pointer to a buffer that will get the full dns name
 *  @param[in] str a pointer to the source c-string
 *
 *  @return Returns the length of the string up to the last '\0'
 */

/* TWO use */

ya_result cstr_to_dnsname(u8* name_parm, const char* str);

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

ya_result cstr_to_dnsname_with_check(u8* name_parm, const char* str);

/**
 *  @brief Converts a C string to a dns rname and checks for validity
 *
 *  Converts a C string to a dns rname.
 *
 *  @param[in] name_parm a pointer to a buffer that will get the full dns name
 *  @param[in] str a pointer to the source c-string
 *
 *  @return Returns the length of the string up to the last '\0'
 */

ya_result cstr_to_dnsrname_with_check(u8* name_parm, const char* str);

static inline int
rr_convert_2dname2(u_char *dst, const u_char *src)
{
    return cstr_to_dnsname((u8*)dst, (const char*)src);
}

/* ONE use */

ya_result cstr_get_dnsname_len(const char* str);

/** @brief Converts a dns name to a C string
 *
 *  Converts a dns name to a C string
 *
 *  @param[in] name a pointer to the source dns name
 *  @param[in] str a pointer to a buffer that will get the c-string
 *
 *  @return Returns the length of the string
 */

/* SIX uses */

u32 dnsname_to_cstr(char* str, const u8* name);

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

bool dnslabel_equals(const u8* name_a, const u8* name_b);

int dnsname_compare(const u8* name_a, const u8* name_b);

/** @brief Tests if two DNS labels are (case-insensitive) equals
 *
 *  Tests if two DNS labels are (case-insensitive) equals
 *
 *  @param[in] name_a a pointer to a lo-case dnsname to compare
 *  @param[in] name_b a pointer to a any-case dnsname to compare
 *
 *  @return Returns TRUE if names are equal, else FALSE.
 */

bool dnslabel_equals_ignorecase_left(const u8* name_a, const u8* name_b);

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

bool dnsname_equals(const u8* name_a, const u8* name_b);

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

bool dnsname_equals_ignorecase(const u8* name_a, const u8* name_b);

/** @brief Returns the full length of a dns name
 *
 *  Returns the full length of a dns name
 *
 *  @param[in] name a pointer to the dnsname
 *
 *  @return The length of the dnsname, "." ( zero ) included
 */

/* SEVENTEEN uses (more or less) */

u32 dnsname_len(const u8* name);

/* ONE use */

u32 dnsname_getdepth(const u8* name);

/* ONE use */

u32 dnsname_copy(u8* dst, const u8* src);

/* malloc & copies a dnsname */

u8* dnsname_dup(const u8* src);

/** @brief Canonizes a dns name.
 *
 *  Canonizes a dns name. (Lo-case)
 *
 *  @param[in] src a pointer to the dns name
 *  @param[out] dst a pointer to a buffer that will hold the canonized dns name
 *
 *  @return The length of the dns name
 */

/* TWELVE uses */

u32 dnsname_canonize(const u8* src, u8* dst);

/**
 * char DNS charset test
 * 
 * @param c
 * @return TRUE iff c in in the DNS charset
 * 
 */

bool dnsname_is_charspace(u8 c);

/**
 * label DNS charset test
 * 
 * @param label
 * @return TRUE iff each char in the label in in the DNS charset
 * 
 */

bool dnslabel_verify_charspace(u8 *label);

/**
 * label DNS charset test and set to lower case
 * 
 * @param label
 * @return TRUE iff each char in the label in in the DNS charset
 * 
 */

bool dnslabel_locase_verify_charspace(u8 *label);


/**
 * dns name DNS charset test and set to lower case
 * 
 * LOCASE is done using |32
 * 
 * @param name_wire
 * @return TRUE iff each char in the name in in the DNS charset
 * 
 */

bool dnsname_locase_verify_charspace(u8 *name_wire);

/**
 * dns name DNS charset test and set to lower case
 * 
 * LOCASE is done using tolower(c)
 * 
 * @param name_wire
 * @return TRUE iff each char in the name in in the DNS charset
 * 
 */

bool dnsname_locase_verify_extended_charspace(u8 *name_wire);

/*****************************************************************************
 *
 * VECTOR
 *
 *****************************************************************************/

/* ONE use */

u32 dnslabel_vector_to_cstr(const_dnslabel_vector_reference name, s32 top, char *str);

/* TWO use */

u32 dnslabel_vector_to_dnsname(const_dnslabel_vector_reference name, s32 top, u8 *str_start);

/* ONE use */

u32 dnslabel_vector_dnslabel_to_dnsname(const u8 *prefix, const dnsname_vector *namestack, s32 bottom, u8 *str);

/* ONE use */

u32 dnsname_vector_sub_to_dnsname(const dnsname_vector *name, s32 from, u8 *name_start);

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

s32 dnsname_to_dnslabel_vector(const u8* dns_name, dnslabel_vector_reference labels);

s32 dnsname_to_dnslabel_stack(const u8* dns_name, dnslabel_stack_reference labels);

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

s32 dnsname_to_dnsname_vector(const u8* dns_name, dnsname_vector* name);

u32 dnsname_vector_copy(dnsname_vector* dst, const dnsname_vector* src);

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

u32 dnslabel_stack_to_cstr(const const_dnslabel_stack_reference name, s32 top, char* str);

/* ONE use */

u32 dnslabel_stack_to_dnsname(const const_dnslabel_stack_reference name, s32 top, u8* str_start);

/* ONE use */

u32 dnsname_stack_to_dnsname(const dnsname_stack* name_stack, u8* name_start);

/* TWO uses (debug) */

u32 dnsname_stack_to_cstr(const dnsname_stack* name, char* str);

/* ONE use */

bool dnsname_equals_dnsname_stack(const u8* str, const dnsname_stack* name);

bool dnsname_under_dnsname_stack(const u8* str, const dnsname_stack* name);

/* FOUR uses */

s32 dnsname_stack_push_label(dnsname_stack* dns_name, u8* dns_label);

/* FOUR uses */

s32 dnsname_stack_pop_label(dnsname_stack* name);

s32 dnsname_to_dnsname_stack(const u8* dns_name, dnsname_stack* name);

#ifdef	__cplusplus
}
#endif

#endif	/* _NAME_H */

/** @} */
