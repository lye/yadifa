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
 *  @brief serial arithmetic
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#include "dnscore/serial.h"

/*
 * SERIAL
 */

/**
 * 
 * Returns TRUE iff a > b in serial arithmetic, else FALSE
 * 
 * @param a
 * @param b
 * @return a > b
 */

bool
serial_gt(u32 a, u32 b)
{
    return ( (a < b) && ((b - a) > 0x7fffffff)) || ((a > b) && ((a - b) < 0x7fffffff));
}

/**
 * 
 * Returns TRUE iff a < b in serial arithmetic, else FALSE
 * 
 * @param a
 * @param b
 * @return a < b
 */


bool
serial_lt(u32 a, u32 b)
{
    return ( (a < b) && ((b - a) < 0x7fffffff)) || ((a > b) && ((a - b) > 0x7fffffff));
}

/**
 * 
 * Returns TRUE iff a >= b in serial arithmetic, else FALSE
 * 
 * @param a
 * @param b
 * @return a >= b
 */

bool
serial_ge(u32 a, u32 b)
{
    return (a == b) || ((a < b) && ((b - a) > 0x7fffffff)) || ((a > b) && ((a - b) < 0x7fffffff));
}

/**
 * 
 * Returns TRUE iff a <= b in serial arithmetic, else FALSE
 * 
 * @param a
 * @param b
 * @return a <= b
 */

bool
serial_le(u32 a, u32 b)
{
    return (a == b) || ((a < b) && ((b - a) < 0x7fffffff)) || ((a > b) && ((a - b) > 0x7fffffff));
}

/** @} */

/*----------------------------------------------------------------------------*/

