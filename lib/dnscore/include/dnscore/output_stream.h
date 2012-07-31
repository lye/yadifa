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
/** @ingroup dnscore
 *
 */
/*----------------------------------------------------------------------------*/
#ifndef _OUTPUT_STREAM_H
#define	_OUTPUT_STREAM_H

#include <dnscore/sys_types.h>
#include <dnscore/dnsname.h>

#ifdef	__cplusplus
extern "C" {
#endif

    typedef struct output_stream output_stream;
    

    typedef ya_result output_stream_write_method(output_stream* stream,const u8* buffer,u32 len);
    typedef ya_result output_stream_flush_method(output_stream* stream);
    
    typedef void output_stream_close_method(output_stream* stream);

    typedef ya_result output_stream_skip_method(output_stream* stream,u32 byte_count);

    typedef struct output_stream_vtbl output_stream_vtbl;
    

    struct output_stream_vtbl
    {
        output_stream_write_method* write;
        output_stream_flush_method* flush;
        output_stream_close_method* close;

        const char* __class__;              /* MUST BE A UNIQUE POINTER, ie: One defined in the class's .c file
                                               The name should be unique in order to avoid compiler tricks
					     */
        
                                            /* Add your inheritable methods here */
    };

    struct output_stream
    {
        void* data;
        output_stream_vtbl* vtbl;
    };

    #define output_stream_class(os) ((os)->vtbl)
    #define output_stream_class_name(os) ((os)->vtbl->__class__)
    #define output_stream_write(os,buffer,len) (os)->vtbl->write(os,buffer,len)
    #define output_stream_flush(os) (os)->vtbl->flush(os)
    #define output_stream_close(os) (os)->vtbl->close(os)
    #define output_stream_skip(os,len) (os)->vtbl->skip(os,len)
    #define output_stream_valid(is) ((is)->vtbl != NULL)

    ya_result output_stream_write_nu32(output_stream* os, u32 value);
    ya_result output_stream_write_nu16(output_stream* os, u16 value);

    /*
     * ya_result output_stream_write_u8(output_stream* os, u8 value);
     */

    static inline ya_result
    output_stream_write_u8(output_stream* os, u8 value)
    {
        return output_stream_write(os, &value, 1);
    }

    ya_result output_stream_write_u16(output_stream* os, u16 value);

    /*
     * PACKED unsigned 32 bits
     *
     * The integer is divided into 7 bits packets (lsb -> msb)
     * The 8th bit is set until the end is reached
     *
     * [  0..  127] => [     0x00 ..      0x7f]
     * [128..16384] => [0x80 0x01 .. 0xff 0x7f]
     *
     */

    ya_result output_stream_write_pu32(output_stream* os, u32 value);

    ya_result output_stream_write_dnsname(output_stream* os, const u8* name);

    ya_result output_stream_write_dnslabel_vector(output_stream* os, dnslabel_vector_reference labels, s32 top);

    ya_result output_stream_write_dnslabel_stack(output_stream* os, dnslabel_stack_reference labels, s32 top);

    ya_result output_stream_decode_base64(output_stream* os, const char * string, u32 length);
    ya_result output_stream_decode_base32(output_stream* os, const char * string, u32 length);
    ya_result output_stream_decode_base32hex(output_stream* os, const char * string, u32 length);
    ya_result output_stream_decode_base16(output_stream* os, const char * string, u32 length);

    /* Found on typebitmap.h */    
    #define output_stream_write_type_bit_maps type_bit_maps_output_stream_write

    output_stream* output_stream_alloc();

/**
 * This tools allows a safer misuse (and detection) of closed streams
 * It sets the stream to a sink that warns abouts its usage and for which every call that can fail fails.
 */

void output_stream_set_void(output_stream *stream);
    
#ifdef	__cplusplus
}
#endif

#endif	/* _OUTPUT_STREAM_H */

