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
/** @defgroup debug Debug functions
 *  @ingroup dnscore
 *  @brief Debug functions settings
 *
 * @{
 */
#ifndef _DEBUG_CONFIG_H
#define	_DEBUG_CONFIG_H

#ifdef	__cplusplus
extern "C"
{
#endif

/**
 * These settings COULD be set by the configure
 * But given the nature of these flags, I don't think it would be a good idea.
 *
 */

/**
 *
 * DEBUG: Enables (1) or disables (0) the internal memory debugging.
 *
 * If ZALLOC is enabled, the memory debugger does not do much.
 *
 * Recommended value: 0
 *
 */

#define ZDB_DEBUG_MALLOC 0
    
#ifdef NDEBUG
#undef ZDB_DEBUG_MALLOC
#define ZDB_DEBUG_MALLOC 0
#endif

#define ZDB_DEBUG_MALLOC_TRASHMEMORY 1

/**
 *
 * DEBUG: Enables (1) or disables (0) the additional memory debugging.
 * This feature has been used to configure the ZMALLOC page sizes for
 * each line size.
 *
 * Recommended value: 1
 */

#define ZDB_DEBUG_ENHANCED_STATISTICS 1

/**
 *
 * DEBUG: Sets the maximum block size (8 bytes granularity) that is taken
 * in account in the ZDB_DEBUG_ENHANCED_STATISTICS==1 mode.
 * Blocks bigger than this are all grouped in a "+++" group.
 *
 * Recommended value: 256
 */

#define ZDB_DEBUG_ENHANCED_STATISTICS_MAX_MONITORED_SIZE 256

/**
 *
 * DEBUG: Enables (1) or disables (0) dumping each time a malloc of a free
 * is called.
 * Recommende value: FALSE
 */

#define ZDB_DEBUG_SHOW_ALLOCS FALSE

/**
 * DEBUG: Sets a limit on the memory available for the database.
 * Debug overhead is not taken in account here.
 *
 * 1GB should be enough.
 *
 * Recommended value: 0x40000000
 *
 */

#define ZDB_DEBUG_ALLOC_MAX 0x80000000

/**
 * DEBUG: Enables block chaining (RECOMMENDED)
 *
 * Recommended value: 1
 */

#define ZDB_DEBUG_CHAIN_ALLOCATED_BLOCKS 1

/**
 * DEBUG: Enable memory block tagging
 *
 * Recommended value: 1
 */

#define ZDB_DEBUG_TAG_BLOCKS 1

/**
 * DEBUG: Each block has got an "unique" serial id of 64 bits.
 *
 * Recommended value: 1
 */

#define ZDB_DEBUG_SERIALNUMBERIZE_BLOCKS 1

#ifdef	__cplusplus
}
#endif

#endif	/* _DEBUG_CONFIG_H */

/** @} */

/*----------------------------------------------------------------------------*/

