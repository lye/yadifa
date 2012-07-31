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
/** @defgroup dnscoretools Generic Tools
 *  @ingroup dnscore
 *  @brief
 *
 * @{
 */

#ifndef _PARSING_H
#define	_PARSING_H

#include <dnscore/sys_types.h>

#ifdef	__cplusplus
extern "C"
{
#endif

/** \brief A string will be checked
 *
 *  The number will be extracted from the string if present. This number can
 *  be 10-based, or hex-based, or...\n
 *  The base must be between 2 and 36 and the number must be be between the min
 *  values and max value
 *
 *  @param[in]  src  string with number part in it
 *  @param[out] dst  number found
 *  @param[in]  min
 *  @param[in]  max
 *  @param[in]  base
 *
 *  @retval OK
 *  @retval NOK, if no digits found, or number not in the range
 */
ya_result
parse_u32_check_range(const char *src, u32 *dst, u32 min, u32 max, u8 base);

/** \brief Converts a chain of pascal strings to a string
 *
 *  Converts a chain of pascal strings to a string
 *
 *  @param[in]  src  string in the form [len+chars]*
 *  @param[out] dst  string
 *
 *  @retval OK
 *  @retval NOK, if something is broken
 */
ya_result
parse_pstring(char **srcp_in_out, size_t src_len, u8 *dst, size_t dst_len);

/** \brief Converts a string to an epoch
 *
 *  Converts a string to an epoch
 *
 *  @param[in]  src  string in the form YYYYMMDDhhmmss
 *  @param[out] dst  value of the source converted into GMT epoch
 *
 *  @retval OK
 *  @retval NOK, if no digits found, or number not in the range
 */
ya_result
parse_yyyymmddhhmmss_check_range(const char *src, u32 *dst);

/** \brief Copies and trim a string
 *
 *  Copies a string while remove head & tail spaces and reducing any blank run to a single space
 *  The source does not need to be asciiz
 *  The destination will be asciiz
 *
 *  @param[in] src      string
 *  @param[in] src_len  size of the string (the zero sentinel is not checked)
 *  @param[in] dst      buffer that will receive the output string
 *  @param[in] dst_len  size of the buffer
 *
 *  @retval >= 0, the length of the dst string
 *  @retval ERROR, dst_len was too small
 */

ya_result
parse_copy_trim_spaces(const char *src, u32 src_len, char *dst, u32 dst_len);

/** \brief Skips a specific keyword from a string, case insensitive
 *
 *  Skips a specific keyword from a string,  case insensitive, skips white spaces before and after the match
 *
 *  @param[in] src          string
 *  @param[in] src_len      size of the string (the zero sentinel is not checked)
 *  @param[in] words        array of strings that will be looked for
 *  @param[in] word_count   the size of the array
 *  @param[in] matched_word a pointer to an integer that will hold the matched word index or -1 (can be NULL)
 *
 *  @retval >= 0, the number of bytes until the next word
 *  @retval ERROR, dst_len was too small
 */

ya_result
parse_skip_word_specific(const char *src, u32 src_len, const char **words, u32 word_count, s32 *matched_word);

/** \brief Skips a specific keyword from a string, case insensitive
 *
 *  Skips a specific keyword from a string,  case insensitive, skips white spaces before and after the match
 *
 *  @param[in] src          string
 *  @param[in] src_len      size of the string (the zero sentinel is not checked)
 *  @param[in] dst          buffer that will receive the binary version of the ip
 *  @param[in] dst_len      the size of the buffer, minimum 4 for ipv4 and minimum 16 for ipv6
 *
 *  @retval >= 0, the number of bytes written (4 for ipv4 and 16 for ipv6)
 *  @retval ERROR, dst_len was too small or the src was not a valid ip
 */

ya_result
parse_ip_address(const char *src, u32 src_len, u8 *dst, u32 dst_len);

#ifdef	__cplusplus
}
#endif

#endif	/* _PARSING_H */

/** @} */
