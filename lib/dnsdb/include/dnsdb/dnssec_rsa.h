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
/** @defgroup dnskey DNSSEC keys functions
 *  @ingroup dnsdbdnssec
 *  @brief 
 *
 * @{
 */
/*----------------------------------------------------------------------------*/
#ifndef _DNSSEC_RSA_H
#define	_DNSSEC_RSA_H
/*------------------------------------------------------------------------------
 *
 * USE INCLUDES */


#include <dnscore/sys_types.h>
#include <dnsdb/dnssec_config.h>
#include <dnsdb/dnssec_task.h>
#include <dnsdb/dnssec.h>
#include <dnsdb/dnssec_keystore.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
RSA*    rsa_dnskey_public_load(u8* rdata,u32 rdata_size);
u32     rsa_dnskey_getsize(RSA* rsa);
u32     rsa_dnskey_store(RSA* rsa, u8* output_buffer,u16 flags);
*/


ya_result rsa_storeprivate(FILE* private, dnssec_key* key);
ya_result rsa_loadpublic(const u8 *rdata, u16 rdata_size, const char *origin, dnssec_key** out_key);
ya_result rsa_loadprivate(FILE* private, u8 algorithm,u16 flags,const char* origin, dnssec_key** out_key);
ya_result rsa_initinstance(RSA* rsa, u8 algorithm,u16 flags,const char* origin, dnssec_key** out_key);
ya_result rsa_newinstance(u32 size, u8 algorithm,u16 flags,const char* origin, dnssec_key** out_key);

#ifdef	__cplusplus
}
#endif

#endif	/* _DNSSEC_RSA_H */

    /*    ------------------------------------------------------------    */

/** @} */

/*----------------------------------------------------------------------------*/
