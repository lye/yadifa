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
/**
 *  @defgroup base Base conversion functions
 *  @ingroup dnscore
 *  @brief Base 32 hex codec
 *
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>

#include "dnscore/base32hex.h"

/*
 *
 */

#define BASE32_HEX_PADDING '='

static const char __BASE32_HEX__[256] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',

    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
};

/**
 * Encodes bytes into base32hex
 * The output size must be at least size_in * 8/5
 * 
 * @param buffer_in     bytes to convert
 * @param size_in       number of bytes
 * @param buffer_out    output buffer of a size >= size_in * 8/5
 * 
 * @return output size
 */

u32
base32hex_encode(const u8* buffer_in, u32 size_in, char* buffer_out)
{
    char* ptr = buffer_out;

    while(size_in >= 5)
    {
        u8 b0 = *buffer_in++;
        u8 b1 = *buffer_in++;
        u8 b2 = *buffer_in++;
        u8 b3 = *buffer_in++;
        u8 b4 = *buffer_in++;

        *ptr++ = __BASE32_HEX__[ b0 >> 3 ];
        *ptr++ = __BASE32_HEX__[(u8)((b0 << 2) | (b1 >> 6))];
        *ptr++ = __BASE32_HEX__[ b1 >> 1 ];
        *ptr++ = __BASE32_HEX__[(u8)((b1 << 4) | (b2 >> 4))];
        *ptr++ = __BASE32_HEX__[(u8)((b2 << 1) | (b3 >> 7))];
        *ptr++ = __BASE32_HEX__[ b3 >> 2 ];
        *ptr++ = __BASE32_HEX__[(u8)((b3 << 3) | (b4 >> 5))];
        *ptr++ = __BASE32_HEX__[ b4 ];

        size_in -= 5;
    }

    switch(size_in)
    {
        case 4:
        {
            u8 b0 = *buffer_in++;
            u8 b1 = *buffer_in++;
            u8 b2 = *buffer_in++;
            u8 b3 = *buffer_in++;

            *ptr++ = __BASE32_HEX__[ b0 >> 3 ];
            *ptr++ = __BASE32_HEX__[(u8)((b0 << 2) | (b1 >> 6))];
            *ptr++ = __BASE32_HEX__[ b1 >> 1 ];
            *ptr++ = __BASE32_HEX__[(u8)((b1 << 4) | (b2 >> 4))];
            *ptr++ = __BASE32_HEX__[(u8)((b2 << 1) | (b3 >> 7))];
            *ptr++ = __BASE32_HEX__[ b3 >> 2 ];

            *ptr++ = __BASE32_HEX__[(u8)(b3 << 3)];
            *ptr++ = BASE32_HEX_PADDING;
            break;
        }
        case 3:
        {
            u8 b0 = *buffer_in++;
            u8 b1 = *buffer_in++;
            u8 b2 = *buffer_in++;

            *ptr++ = __BASE32_HEX__[ b0 >> 3 ];
            *ptr++ = __BASE32_HEX__[(u8)((b0 << 2) | (b1 >> 6))];
            *ptr++ = __BASE32_HEX__[ b1 >> 1 ];
            *ptr++ = __BASE32_HEX__[(u8)((b1 << 4) | (b2 >> 4))];

            *ptr++ = __BASE32_HEX__[(u8)(b2 << 1)];
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            break;
        }
        case 2:
        {
            u8 b0 = *buffer_in++;
            u8 b1 = *buffer_in++;

            *ptr++ = __BASE32_HEX__[ b0 >> 3 ];
            *ptr++ = __BASE32_HEX__[(u8)((b0 << 2) | (b1 >> 6))];
            *ptr++ = __BASE32_HEX__[ b1 >> 1 ];

            *ptr++ = __BASE32_HEX__[(u8)(b1 << 4)];
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            break;
        }
        case 1:
        {
            u8 b0 = *buffer_in++;

            *ptr++ = __BASE32_HEX__[ b0 >> 3 ];
            *ptr++ = __BASE32_HEX__[(u8)(b0 << 2) ];

            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            *ptr++ = BASE32_HEX_PADDING;
            break;
        }
    }

    return (u32)(ptr - buffer_out);
}

#define __DEBASE32_HEX__STOP__ 0x80

static const u8 __DEBASE32_HEX__[256] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  0 -  7 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /*  8 - 15 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 16 - 23 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 24 - 31 */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* 32 - 39 */
    0xff, 0xff, 0xff, 0x3e, 0xff, 0xff, 0xff, 0x3f, /* 40 - 47 ...+.../ */
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, /* 01234567 */
    0x08, 0x09, 0xff, 0xff, 0xff, 0x80, 0xff, 0xff, /* 89...=.. */

    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, /* .ABCDEFG */
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, /* HIJKLMNO */
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0xff, /* PQRSTUVW */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* XYZ..... */
    0xff, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, /* .abcdefg */ /* Added to support NSD */
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, /* hijklmno */
    0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0xff, /* pqrstuvw */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* xyz..... */

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/**
 * Decodes base32hex into bytes
 * The output size must be at least size_in * 5/8
 * 
 * @param buffer_in     base32hex text
 * @param size_in       number of chars
 * @param buffer_out    output buffer of a size >= size_in * 5/8
 * 
 * @return output size
 */

ya_result
base32hex_decode(const char* buffer_in, u32 size_in, u8* buffer_out)
{
    zassert((size_in & 3) == 0);

    u8* in = (u8*)buffer_in;
    u8* out = buffer_out;

    while(size_in > 8)
    {
        u8 a = __DEBASE32_HEX__[*in++];
        u8 b = __DEBASE32_HEX__[*in++];
        u8 c = __DEBASE32_HEX__[*in++];
        u8 d = __DEBASE32_HEX__[*in++];
        u8 e = __DEBASE32_HEX__[*in++];
        u8 f = __DEBASE32_HEX__[*in++];
        u8 g = __DEBASE32_HEX__[*in++];
        u8 h = __DEBASE32_HEX__[*in++];

        if(((a | b | c | d | e | f | g | h)&0x40) != 0x00)
        {
            /* PARSE ERROR */

            return PARSEB32H_ERROR;
        }

        *out++ = (a << 3) | (b >> 2);
        *out++ = (b << 6) | (c << 1) | (d >> 4);
        *out++ = (d << 4) | (e >> 1);
        *out++ = (e << 7) | (f << 2) | (g >> 3);
        *out++ = (g << 5) | h;

        size_in -= 8;
    }

    if(size_in != 0) /* It's either 0 or 4 */
    {
        u8 a = __DEBASE32_HEX__[*in++];
        u8 b = __DEBASE32_HEX__[*in++];

        if(((a | b)&0xc0) != 0x00)
        {
            /* PARSE ERROR */

            return PARSEB32H_ERROR;
        }

        *out++ = (a << 3) | (b >> 2);

        u8 c = __DEBASE32_HEX__[*in++];
        u8 d = __DEBASE32_HEX__[*in++];

        if((c != __DEBASE32_HEX__STOP__) && (d != __DEBASE32_HEX__STOP__))
        {
            if(((c | d)&0x40) != 0)
            {
                return PARSEB32H_ERROR;
            }

            *out++ = (b << 6) | (c << 1) | (d >> 4);

            u8 e = __DEBASE32_HEX__[*in++];

            if(e != __DEBASE32_HEX__STOP__)
            {
                if((e & 0x40) != 0)
                {
                    return PARSEB32H_ERROR;
                }

                *out++ = (d << 4) | (e >> 1);

                u8 f = __DEBASE32_HEX__[*in++];
                u8 g = __DEBASE32_HEX__[*in++];

                if((f != __DEBASE32_HEX__STOP__) && (g != __DEBASE32_HEX__STOP__))
                {
                    if(((f | g)&0x40) != 0)
                    {
                        return PARSEB32H_ERROR;
                    }

                    *out++ = (e << 7) | (f << 2) | (g >> 3);

                    u8 h = __DEBASE32_HEX__[*in++];

                    if(h != __DEBASE32_HEX__STOP__)
                    {
                        if((h & 0x40) != 0)
                        {
                            return PARSEB32H_ERROR;
                        }

                        *out++ = (g << 5) | h;
                    }
                }
            }
        }
    }

    return out - buffer_out;
}

/**
 * encodes the buffer into base32hex to the output stream
 * 
 * @param os        output stream
 * @param buffer_in buffer to encode
 * @param size_in   size of the buffer
 * 
 * @return bytes written
 */

ya_result
output_stream_write_base32hex(output_stream* os, const u8* buffer_in, u32 size_in)
{
    ya_result total = ((size_in + (BASE32HEX_DECODED_CHUNK - 1)) / BASE32HEX_DECODED_CHUNK) * BASE32HEX_ENCODED_CHUNK;

    char tmp[8];

    while(size_in >= BASE32HEX_DECODED_CHUNK)
    {
        /* this cannot fail */
        base32hex_encode(buffer_in, BASE32HEX_DECODED_CHUNK, tmp);

        output_stream_write(os, (u8*)tmp, BASE32HEX_ENCODED_CHUNK);
        buffer_in += BASE32HEX_DECODED_CHUNK;
        size_in -= BASE32HEX_DECODED_CHUNK;
    }

    ya_result return_code;

    /* doing the general case in the if block results into faster code */

    if(ISOK(return_code = output_stream_write(os, (u8*)tmp, base32hex_encode(buffer_in, size_in, tmp))))
    {
        return total;
    }

    return return_code;
}

/** @} */

/*----------------------------------------------------------------------------*/

