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
/** @defgroup systemtypes Definition of types in order to ensure architecture-independence
 *  @ingroup dnscore
 *  @brief Definition of types in order to ensure architecture-independence
 *
 * @{
 */

#ifndef _SYSTYPES_H
#define	_SYSTYPES_H

#include <dnscore/dnscore-config.h>

#ifndef DNSCORE_BUILD

#undef VERSION
#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_TARNAME
#undef PACKAGE_STRING
#undef PACKAGE_VERSION

#endif

#define HAS_ATOMIC_FEATURES 1

#include <stdlib.h>

#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <dnscore/sys_error.h>

#include <sys/types.h> /** @note must be used for u_char on Mac OS X */

#ifdef	__cplusplus
extern "C"
{
#endif
    
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#if defined(__bool_true_false_are_defined) && __bool_true_false_are_defined != 0

#ifndef TRUE
#define TRUE  true
#endif

#ifndef FALSE
#define FALSE false
#endif

#else

typedef int bool;

#ifndef TRUE
#define TRUE  (0==0)
#endif

#ifndef FALSE
#define FALSE (0==1)
#endif

#endif


/* This is the basic type definition set                        */
/* Tweaks will be added for each setup (using the preprocessor) */

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;
#if defined(HAVE_UINT64_T) && defined(HAVE_INT64_T)
typedef uint64_t u64;
typedef int64_t s64;
#elif defined(HAVE_LONG_LONG)
typedef unsigned long long u64
typedef signed long long s64
#else
#error NO UNSIGNED 64 BITS TYPE KNOWN ON THIS 64 BITS ARCHITECTURE (u64 + s64)
#endif

typedef void callback_function(void*);

/*
AIX : __64BIT__
HP: __LP64__
SUN: __sparcv9, _LP64
SGI: _MIPS_SZLONG==64
NT: _M_IA64
 */

#ifndef __SIZEOF_POINTER__
#if defined(__LP64__)||defined(__LP64)||defined(_LP64)||defined(__64BIT__)||defined(MIPS_SZLONG)||defined(_M_IA64)
#define __SIZEOF_POINTER__ 8
#else
#define __SIZEOF_POINTER__ 4
#endif
#endif

#if __SIZEOF_POINTER__==4
typedef unsigned int intptr;
#elif __SIZEOF_POINTER__==8
#if defined(HAVE_UINT64_T)
typedef uint64_t intptr;
#elif defined(HAVE_LONG_LONG)
typedef unsigned long long intptr;
#else
#error NO UNSIGNED 64 BITS TYPE KNOWN ON THIS 64 BITS ARCHITECTURE (intptr)
#endif
#else
#error __SIZEOF_POINTER__ value not handled (only 4 and 8 are)
#endif

/**
 * This macro returns the first address aligned to 8 bytes from the parameter
 * addresss.
 * 
 */

#define ALIGN8(__from_address__) (((__from_address__)+7)&~7)

/**
 * This macro returns the first address aligned to 16 bytes from the parameter
 * addresss.
 * 
 */

#define ALIGN16(__from_address__) (((__from_address__)+15)&~15)

/*
 * Macros used to access bytes inside a buffer.
 *
 * ie: U32_AT(packet[8]), assuming that packet is a byte buffer, will access
 *     the 8th byte inside packet as a (native) unsigned 32bits integer.
 */

#define U8_AT(address)  (*((u8*)&(address)))

#if HAS_MEMALIGN_ISSUES == 0

#define GET_U16_AT(address) (*((u16*)&(address)))
#define SET_U16_AT(address,value) *((u16*)&(address))=(value);

#define GET_U32_AT(address) (*((u32*)&(address)))
#define SET_U32_AT(address,value) *((u32*)&(address))=(value);

#define GET_U64_AT(address) (*((u64*)&(address)))
#define SET_U64_AT(address,value) *((u64*)&(address))=(value);

#else /* sparc ... */

/*
 *  Why in caps ? Traditionnaly it was an helper macro.  Macros are in caps except when they hide a virtual call.
 *
 */

static inline u16 GET_U16_AT_P(const void* p)
{
    const u8* p8=(const u8*)p;
    u16 v;

#ifdef WORDS_BIGENDIAN
    v=p8[0];
    v<<=8;
    v|=p8[1];
#else
    v=p8[1];
    v<<=8;
    v|=p8[0];
#endif
    
    return v;
}

#define GET_U16_AT(x) GET_U16_AT_P(&(x))

static inline void SET_U16_AT_P(void* p, u16 v)
{
    u8* p8=(u8*)p;

#ifdef WORDS_BIGENDIAN
    p8[0]=v>>8;
    p8[1]=v;
#else
    p8[0]=v;
    p8[1]=v>>8;
#endif
}

#define SET_U16_AT(x___,y___) SET_U16_AT_P(&(x___),(y___))

static inline u32 GET_U32_AT_P(const void* p)
{
    const u8* p8=(const u8*)p;
    u32 v;

#ifdef WORDS_BIGENDIAN
    v=p8[0];
    v<<=8;
    v|=p8[1];
    v<<=8;
    v|=p8[2];
    v<<=8;
    v|=p8[3];
#else
    v=p8[3];
    v<<=8;
    v|=p8[2];
    v<<=8;
    v|=p8[1];
    v<<=8;
    v|=p8[0];
#endif
    return v;
}

#define GET_U32_AT(x) GET_U32_AT_P(&(x))

static inline void SET_U32_AT_P(void* p, u32 v)
{
    u8* p8=(u8*)p;

#ifdef WORDS_BIGENDIAN
    p8[0]=v>>24;
    p8[1]=v>>16;
    p8[2]=v>>8;
    p8[3]=v;
#else
    p8[0]=v;
    p8[1]=v>>8;
    p8[2]=v>>16;
    p8[3]=v>>24;
#endif
}

#define SET_U32_AT(x___,y___) SET_U32_AT_P(&(x___),(y___))

static inline u64 GET_U64_AT_P(const void* p)
{
    const u8* p8=(const u8*)p;
    u32 v;

#ifdef WORDS_BIGENDIAN
    v=p8[0];
    v<<=8;
    v|=p8[1];
    v<<=8;
    v|=p8[2];
    v<<=8;
    v|=p8[3];
    v<<=8;
    v|=p8[4];
    v<<=8;
    v|=p8[5];
    v<<=8;
    v|=p8[6];
    v<<=8;
    v|=p8[7];
#else
    v=p8[7];
    v<<=8;
    v|=p8[6];
    v<<=8;
    v|=p8[5];
    v<<=8;
    v|=p8[4];
    v<<=8;
    v=p8[3];
    v<<=8;
    v|=p8[2];
    v<<=8;
    v|=p8[1];
    v<<=8;
    v|=p8[0];
#endif
    return v;
}

#define GET_U64_AT(x) GET_U64_AT_P(&(x))

static inline void SET_U64_AT_P(void* p, u64 v)
{
    u8* p8=(u8*)p;

#ifdef WORDS_BIGENDIAN
    p8[0]=v>>56;
    p8[1]=v>>48;
    p8[2]=v>>40;
    p8[3]=v>>32;
    p8[4]=v>>24;
    p8[5]=v>>16;
    p8[6]=v>>8;
    p8[7]=v;
#else
    p8[0]=v;
    p8[1]=v>>8;
    p8[2]=v>>16;
    p8[3]=v>>24;
    p8[4]=v>>32;
    p8[5]=v>>40;
    p8[6]=v>>48;
    p8[7]=v>>56;
#endif
}

#define SET_U64_AT(x___,y___) SET_U64_AT_P(&(x___),(y___))


#endif

#define MAX_U8  ((u8)0xff)
#define MAX_U16 ((u16)0xffff)
#define MAX_U32 ((u32)0xffffffffL)
#define MAX_U64 ((u64)0xffffffffffffffffLL)

#define MAX_S8  ((s8)0x7f)
#define MAX_S16 ((s16)0x7fff)
#define MAX_S32 ((s32)0x7fffffffL)
#define MAX_S64 ((s64)0x7fffffffffffffffLL)

#define MIN_S8  ((s8)0xff)
#define MIN_S16 ((s16)0xffff)
#define MIN_S32 ((s32)0xffffffffL)
#define MIN_S64 ((s64)0xffffffffffffffffLL)

/**/

typedef u32 process_flags_t;

#ifdef WORDS_BIGENDIAN
#define NU16(value)     ((u16)(value))
#define NU32(value)     ((u32)(value))
#else
#define NU16(value)     (u16)(((((u16)(value))>>8)&0xff)|(((u16)(value))<<8))
#define NU32(value)     (u32)(((((u32)(value))>>24)&0xff)|((((u32)(value))>>8)&0xff00)|((((u32)(value))<<8)&0xff00000)|(((u32)(value))<<24))
#endif

#define NETWORK_ONE_16  NU16(0x0001)
#define IS_WILD_LABEL(u8dnslabel_) ( GET_U16_AT(*(u16*)(u8dnslabel_)) == NU16(0x012a))       /* 01 2a = 1 '*' */

/* sys_types.h is included everywhere.  This ensure the debug hooks will be too. */

#define MALLOC_OR_DIE(cast,target,size,tag) if(((target)=(cast)malloc(size))==NULL){perror(__FILE__);exit(EXIT_CODE_OUTOFMEMORY_ERROR);}
#define REALLOC_OR_DIE(cast,src_and_target,newsize,tag) if(((src_and_target)=(cast)realloc((src_and_target),(newsize)))==NULL){perror(__FILE__);exit(EXIT_CODE_OUTOFMEMORY_ERROR);}

#include <dnscore/debug.h>

#define MIN(a,b) (((a)<=(b))?(a):(b))
#define MAX(a,b) (((a)>=(b))?(a):(b))
#define BOUND(a,b,c) (((b)<=(a))?(a):(((b)>=(c))?(c):(b)))

#define ZEROMEMORY(buffer__,size__) memset(buffer__, 0, size__)
#define MEMCOPY(target__,source__,size__) memcpy((target__),(source__),(size__))

struct type_class_ttl_rdlen /* @TODO define at a more appropriate place */
{
    u16 qtype;
    u16 qclass;
    u32 ttl;
    u16 rdlen;
};

#if USES_ICC == 1

#ifndef inline
#define inline __inline
#endif

#else

#ifndef inline
#define inline __inline__
#endif

#endif

#ifdef	__cplusplus
}
#endif



#endif	/* _DBTYPES_H */

/** @} */
