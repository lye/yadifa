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
/** @defgroup streaming Streams
 *  @ingroup dnscore
 *  @brief 
 *
 *  
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#ifndef _INPUT_STREAM_H
#define	_INPUT_STREAM_H

#include <dnscore/sys_types.h>

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct input_stream input_stream;
    

    typedef ya_result input_stream_read_method(input_stream* stream,u8* in_buffer,u32 in_len);
    typedef void input_stream_close_method(input_stream* stream);

    typedef ya_result input_stream_skip_method(input_stream* stream,u32 byte_count);

    typedef struct input_stream_vtbl input_stream_vtbl;
    

    struct input_stream_vtbl
    {
        input_stream_read_method*  read;
        input_stream_skip_method*  skip;
        input_stream_close_method* close;
        const char* __class__;              /* MUST BE A UNIQUE POINTER, ie: One defined in the class's .c file */
                                            /* The name should be unique in order to avoid compiler tricks	*/

                                            /* Add your inheritable methods here    */
    };

    struct input_stream
    {
        void* data;
        input_stream_vtbl* vtbl;
    };

    #define input_stream_class(is_) ((is_)->vtbl)
    #define input_stream_class_name(is_) ((is_)->vtbl->__class__)
    #define input_stream_read(is_,buffer_,len_) (is_)->vtbl->read(is_,buffer_,len_)
    #define input_stream_close(is_) (is_)->vtbl->close(is_)
    #define input_stream_skip(is_,len_) (is_)->vtbl->skip(is_,len_)
    #define input_stream_valid(is_) ((is_)->vtbl != NULL)

    ya_result input_stream_read_fully(input_stream* stream, u8* buffer, u32 len);
    ya_result input_stream_skip_fully(input_stream* stream, u32 len_start);

    ya_result input_stream_read_nu32(input_stream* stream,u32* output);
    ya_result input_stream_read_nu16(input_stream* stream,u16* output);
    ya_result input_stream_read_u32(input_stream* stream,u32* output);
    ya_result input_stream_read_u16(input_stream* stream,u16* output);
    ya_result input_stream_read_u8(input_stream* stream,u8* output);
    
    ya_result input_stream_read_dnsname(input_stream* stream,u8* output);
    
    ya_result input_stream_read_rname(input_stream* stream, u8* output_buffer);
    
    ya_result input_stream_read_line(input_stream* stream, char *output, int max_len);

    static inline ya_result input_stream_read_rr_header(input_stream* is, u8* rname, u32 rname_size, u16* rtype, u16* rclass, u32* rttl, u16* rdata_size)
    {
	ya_result ret;

	if(FAIL(ret = input_stream_read_dnsname(is, rname/*, rname_size*/)))
	{
	    return ret;
	}

	if(FAIL(ret = input_stream_read_fully(is, (u8*)rtype, 2)))   /** @note NATIVETYPE */
	{
	    return ret;
	}

	if(FAIL(ret = input_stream_read_fully(is, (u8*)rclass, 2))) /** @note NATIVECLASS */
	{
	    return ret;
	}

	if(FAIL(ret = input_stream_read_nu32(is, rttl)))
	{
	    return ret;
	}

	return input_stream_read_nu16(is, rdata_size);
    }

/**
 * This tools allows a safer misuse (and detection) of closed streams
 * It sets the stream to a sink that warns abouts its usage and for which every call that can fail fails.
 */

void input_stream_set_void(input_stream* stream);

#ifdef	__cplusplus
}
#endif

#endif	/* _INPUT_STREAM_H */
/** @} */

/*----------------------------------------------------------------------------*/

