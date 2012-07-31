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
/** @defgroup acl Access Control List
 *  @ingroup yadifad
 *  @brief
 *
 * @{
 */

#ifndef _ACL_H
#define	_ACL_H

#include <dnscore/ptr_vector.h>
#include <dnscore/message.h>
#include <dnscore/host_address.h>

#ifdef	__cplusplus
extern "C"
{
#endif

    #define ACL_ERROR_BASE			0x80060000
    #define ACL_ERROR_CODE(code_)		((s32)(ACL_ERROR_BASE+(code_)))
    
    #define ACL_TOKEN_SIZE_ERROR		ACL_ERROR_CODE(1)
    #define ACL_UNEXPECTED_NEGATION		ACL_ERROR_CODE(2)
    #define ACL_UNEXPECTED_MASK    		ACL_ERROR_CODE(3)
    #define ACL_WRONG_V4_MASK			ACL_ERROR_CODE(3)
    #define ACL_WRONG_V6_MASK			ACL_ERROR_CODE(4)
    #define ACL_WRONG_MASK              ACL_ERROR_CODE(5)
    #define ACL_DUPLICATE_ENTRY		  	ACL_ERROR_CODE(6)
    #define ACL_RESERVED_KEYWORD		ACL_ERROR_CODE(7)
    #define ACL_TOO_MUCH_TOKENS         ACL_ERROR_CODE(8)

    #define ACL_REJECTED_BY_IPV4		ACL_ERROR_CODE(101)
    #define ACL_REJECTED_BY_IPV6		ACL_ERROR_CODE(102)
    #define ACL_REJECTED_BY_TSIG		ACL_ERROR_CODE(103)
    #define ACL_UPDATE_REJECTED         ACL_ERROR_CODE(104)
    #define ACL_NOTIFY_REJECTED         ACL_ERROR_CODE(105)

    #define ACL_NAME_PARSE_ERROR		ACL_ERROR_CODE(201)
    #define ACL_UNKNOWN_TSIG_KEY		ACL_ERROR_CODE(202)
    
    /* acl can be identified by
     * ipv4 (accept/reject)
     * ipv6 (accept/reject)
     * TSIG (accept only)
     * => 3 distinct entry points
     * Tests will be done first on IP (if any, and by version) then on TSIG (if any)
     *
     */

    typedef struct ipv4_id ipv4_id;

    struct ipv4_id
    {
        addressv4 address;
        addressv4 mask;
        s8        maskbits; // needs to be signed
        s8        rejects;
    };

    typedef struct ipv6_id ipv6_id;

    struct ipv6_id
    {
        addressv6 address;
        addressv6 mask;
        s16       maskbits; // needs to be signed
        s8        rejects;
    };

    #define TSIG_SECRET_MAX_SIZE    32

    typedef struct tsig_id tsig_id;

    struct tsig_id
    {
        u8 secret_size;
        u8 name_size;
        u8 mac_algorithm;
        u8 reserved;
    	u8 secret[TSIG_SECRET_MAX_SIZE];
        u8 name[MAX_DOMAIN_LENGTH];
    };

    typedef struct ref_id ref_id;
    
    struct ref_id
    {
        const char* name;
        bool mark;
    };

    /*
     * I've choosen these values to avoid some tests
     * 
     * Basically, there are 2 tests for ACL: IP & TSIG
     * 
     * First IP, then, if available in the message, TSIG
     * 
     * IP will trigger a return in case of reject, but else will worth 0 or -2
     * Then TSIG, if available, will worth -2,0,+2
     * What we want is:
     *      IAIAIA
     *      RRIIAA
     * =>   RRRAAA
     * 
     * With these values we can simply sum instead of doing tests
     *      0 +2  0 +2  0 +2
     *  +  -4 -4  0  0 +2 +2
     *  =  -4 -2  0 +2 +2 +4
     * then remove 1
     *  =  -3 -1 -1 +1 +1 +3
     *  =>  R  R  R  A  A  A
     * 
     */
    
    #define AMIM_ACCEPT  2
    #define AMIM_SKIP    0
    #define AMIM_REJECT -4 // Reject has much more weight, and in theory -2 should be enough ( 1 IPV4/6, 1 TSIG )

    #define ACL_SORT_RULES      0
    #define ACL_MERGE_RULES     0
    #define ACL_DEFAULT_RULE    AMIM_REJECT

    #define ACL_REJECTED(__amim_code__) ((__amim_code__) < 0)
    #define ACL_ACCEPTED(__amim_code__) ((__amim_code__) > 0)
    #define ACL_IGNORED(__amim_code__) ((__amim_code__) == 0)


    /**
     *  Returns:
     *	    > 0 : Accept: matched and accepted
     *      < 0 : Reject: matched and rejected
     *      = 0 : Skip  : not matched
     */

    struct address_match_item;

    typedef int address_match_item_matcher(struct address_match_item*, void* );

    typedef struct address_match_item address_match_item;

    /* Rules like the ones in the ACL named rules set */

    struct address_match_item
    {
        address_match_item_matcher *match;
	
        union
        {
            ipv4_id ipv4;
            ipv6_id ipv6;
            tsig_id tsig;
            ref_id  ref;
        } parameters;
    };

    typedef struct address_match_list address_match_list;

    struct address_match_list
    {
        address_match_item **items;
        address_match_item **limit;  /* Address limit of the items ( p = items; while(p<items) {process(p);} ) */
    };

    typedef struct acl_entry acl_entry;

    struct acl_entry
    {
        address_match_list list;
        const char* name;
    };

    typedef struct address_match_set address_match_set;

    struct address_match_set
    {
        address_match_list ipv4;
        address_match_list ipv6;
        address_match_list tsig;
    };

    typedef struct access_control access_control;

    struct access_control   /* NULL if the list is empty */
    {
        address_match_set allow_query;
        address_match_set allow_update;
        address_match_set allow_update_forwarding;
        address_match_set allow_transfer;
        address_match_set allow_notify;
        address_match_set allow_control;
    };

    /* Add one line into the acl */
    ya_result acl_add_definition(const char* name, const char *description);

    void acl_free_definitions();


    /**
     * Builds an access control using the text descriptors and the acl data.
     * Expands the access control.
     * 
     */

    ya_result acl_build_access_control(	access_control *ac,
                                        const char *allow_query,
                                        const char *allow_update,
                                        const char *allow_update_forwarding,
                                        const char *allow_transfer,
                                        const char *allow_notify,
                                        const char *allow_control);

    void acl_empties_access_control(access_control *ac);
    
    void acl_copy_address_match_set(address_match_set *target, address_match_set *ams);
    void acl_copy_access_control(access_control *target, access_control *ac);

    ya_result acl_build_access_control_item(address_match_set *aci, const char* allow_whatever);

    void acl_merge_access_control(access_control *dest, const access_control *src);

    void acl_unmerge_access_control(access_control *dest, const access_control *src);

    void acl_empties_address_match_set(address_match_set *ams);

    bool acl_address_match_set_isempty(address_match_set *ams);

    typedef ya_result acl_check_access_filter_callback(message_data *mesg, address_match_set *ams);
    
    typedef ya_result acl_query_access_filter_callback(message_data *mesg, void *extension);

    ya_result acl_check_access_filter(message_data *mesg, address_match_set *ams);
    ya_result acl_query_access_filter(message_data *mesg, void *extension);

    /*
     * Returns the filter appropriate for processing the set
     */

    acl_check_access_filter_callback *acl_get_check_access_filter(address_match_set *set);
    acl_query_access_filter_callback *acl_get_query_access_filter(address_match_set *set);
    
    ya_result acl_address_match_item_to_stream(output_stream *os, address_match_item *ami);    
    ya_result acl_address_match_item_to_string(address_match_item *ami, char *out_txt, u32 *out_txt_lenp);
    
    void acl_address_match_set_to_stream(output_stream *os, address_match_set *ams);
    

#ifdef	__cplusplus
}
#endif

#endif	/* _ACL_H */

/** @} */
