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
/** @defgroup name Functions used to manipulate dns formatted names and labels
 *  @ingroup dnsdb
 *  @brief Functions used to manipulate dns formatted names and labels
 *
 * @{
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "dnsdb/zdb_alloc.h"
#include <dnscore/dnsname.h>

/** @brief Allocates and duplicates a name.
 *
 *  Allocates and duplicates a name.
 *
 *  @param[in] name a pointer to the dnsname
 *
 *  @return A new instance of the dnsname.
 */


u8*
dnsname_zdup(const u8* name)
{
    zassert(name != NULL);

    u32 len = dnsname_len(name);

    u8* dup;

    ZALLOC_STRING_OR_DIE(u8*, dup, len, ZDB_NAME_TAG);
    MEMCOPY(dup, name, len);

    return dup;
}

/** @brief Allocates and duplicates a label.
 *
 *  Allocates and duplicates a label.
 *
 *  @param[in] name a pointer to the label
 *
 *  @return A new instance of the label
 */

u8*
dnslabel_dup(const u8* name)
{
    zassert(name != NULL);

    u32 len = name[0] + 1;

    u8* dup;
    ZALLOC_STRING_OR_DIE(u8*, dup, len, ZDB_LABEL_TAG);
    MEMCOPY(dup, name, len);

    return dup;
}

/** @} */
