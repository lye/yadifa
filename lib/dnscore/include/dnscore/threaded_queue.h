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
/** @defgroup threading Threading, pools, queues, ...
 *  @ingroup dnscore
 *  @brief 
 *
 *  
 *
 * @{
 *
 *----------------------------------------------------------------------------*/
#ifndef _THREADED_QUEUE_H
#define	_THREADED_QUEUE_H

/*
 * Four implementations of the threaded queue can be used ...
 */

#define THEADED_QUEUE_RINGLIST      1
#define THEADED_QUEUE_RINGBUFFER    2
#define THEADED_QUEUE_RINGBUFFER_CW 3
#define THEADED_QUEUE_NBRB          4

#define THREADED_QUEUE_MODE THEADED_QUEUE_RINGBUFFER_CW

#if THREADED_QUEUE_MODE == THEADED_QUEUE_RINGLIST
#define THREADED_QUEUE ringlist
#include <dnscore/threaded_ringlist.h>

typedef struct threaded_ringlist threaded_queue;

#define THREADED_QUEUE_NULL THREADED_RINGLIST_NULL

#define threaded_queue_init(queue_,max_size_) threaded_ringlist_init((queue_),(max_size_))
#define threaded_queue_finalize(queue_) threaded_ringlist_finalize((queue_))
#define threaded_queue_enqueue(queue_,constant_pointer_) threaded_ringlist_enqueue((queue_),(constant_pointer_))
#define threaded_queue_try_enqueue(queue_,constant_pointer_) threaded_ringlist_try_enqueue((queue_),(constant_pointer_))
#define threaded_queue_dequeue(queue_) threaded_ringlist_dequeue((queue_))
#define threaded_queue_try_dequeue(queue_) threaded_ringlist_try_dequeue((queue_))
#define threaded_queue_dequeue_set(queue_, array_, size_) threaded_ringlist_dequeue_set((queue_),(array_),(size_))
#define threaded_queue_wait_empty(queue_) threaded_ringlist_wait_empty((queue_))
#define threaded_queue_size(queue_) threaded_ringlist_size((queue_))
/*
 * The queue will block (write) if bigger than this.
 * Note that if the key is already bigger it will blocked (write) until
 * the content is emptied by the readers.
 */
#define threaded_queue_set_maxsize(queue_, max_size_) threaded_ringlist_set_maxsize((queue_), (max_size_))

#elif THREADED_QUEUE_MODE == THEADED_QUEUE_RINGBUFFER
#define THREADED_QUEUE ringbuffer
#include <dnscore/threaded_ringbuffer.h>

typedef struct threaded_ringbuffer threaded_queue;

#define THREADED_QUEUE_NULL THREADED_RINGBUFFER_NULL

#define threaded_queue_init(queue_,max_size_) threaded_ringbuffer_init((queue_),(max_size_))
#define threaded_queue_finalize(queue_) threaded_ringbuffer_finalize((queue_))
#define threaded_queue_enqueue(queue_,constant_pointer_) threaded_ringbuffer_enqueue((queue_),(constant_pointer_))
#define threaded_queue_try_enqueue(queue_,constant_pointer_) threaded_ringbuffer_try_enqueue((queue_),(constant_pointer_))
#define threaded_queue_dequeue(queue_) threaded_ringbuffer_dequeue((queue_))
#define threaded_queue_try_dequeue(queue_) threaded_ringbuffer_try_dequeue((queue_))
#define threaded_queue_dequeue_set(queue_, array_, size_) threaded_ringbuffer_dequeue_set((queue_),(array_),(size_))
#define threaded_queue_wait_empty(queue_) threaded_ringbuffer_wait_empty((queue_))
#define threaded_queue_size(queue_) threaded_ringbuffer_size((queue_))
/*
 * The queue will block (write) if bigger than this.
 * Note that if the key is already bigger it will blocked (write) until
 * the content is emptied by the readers.
 */
#define threaded_queue_set_maxsize(queue_, max_size_) threaded_ringbuffer_set_maxsize((queue_), (max_size_))

#elif THREADED_QUEUE_MODE == THEADED_QUEUE_RINGBUFFER_CW

#define THREADED_QUEUE ringbuffer_cw
#include <dnscore/threaded_ringbuffer_cw.h>

typedef struct threaded_ringbuffer_cw threaded_queue;

#define THREADED_QUEUE_NULL THREADED_RINGBUFFER_CW_NULL

#define threaded_queue_init(queue_,max_size_) threaded_ringbuffer_cw_init((queue_),(max_size_))
#define threaded_queue_finalize(queue_) threaded_ringbuffer_cw_finalize((queue_))
#define threaded_queue_enqueue(queue_,constant_pointer_) threaded_ringbuffer_cw_enqueue((queue_),(constant_pointer_))
#define threaded_queue_try_enqueue(queue_,constant_pointer_) threaded_ringbuffer_cw_try_enqueue((queue_),(constant_pointer_))
#define threaded_queue_peek(queue_) threaded_ringbuffer_cw_peek((queue_))
#define threaded_queue_try_peek(queue_) threaded_ringbuffer_cw_try_peek((queue_))
#define threaded_queue_dequeue(queue_) threaded_ringbuffer_cw_dequeue((queue_))
#define threaded_queue_try_dequeue(queue_) threaded_ringbuffer_cw_try_dequeue((queue_))
#define threaded_queue_dequeue_set(queue_, array_, size_) threaded_ringbuffer_cw_dequeue_set((queue_),(array_),(size_))
#define threaded_queue_wait_empty(queue_) threaded_ringbuffer_cw_wait_empty((queue_))
#define threaded_queue_size(queue_) threaded_ringbuffer_cw_size((queue_))
/*
 * The queue will block (write) if bigger than this.
 * Note that if the key is already bigger it will blocked (write) until
 * the content is emptied by the readers.
 */
#define threaded_queue_set_maxsize(queue_, max_size_) threaded_ringbuffer_cw_set_maxsize((queue_), (max_size_))

#elif THREADED_QUEUE_MODE == THEADED_QUEUE_NBRB

#define THREADED_QUEUE nbrb
#include <dnscore/threaded_nbrb.h>

typedef struct threaded_nbrb threaded_queue;

#define THREADED_QUEUE_NULL THREADED_NBRB_NULL

#define threaded_queue_init(queue_,max_size_) threaded_nbrb_init((queue_),(max_size_))
#define threaded_queue_finalize(queue_) threaded_nbrb_finalize((queue_))
#define threaded_queue_enqueue(queue_,constant_pointer_) threaded_nbrb_enqueue((queue_),(constant_pointer_))
#define threaded_queue_try_enqueue(queue_,constant_pointer_) threaded_nbrb_try_enqueue((queue_),(constant_pointer_))
#define threaded_queue_peek(queue_) threaded_nbrb_peek((queue_))
#define threaded_queue_try_peek(queue_) threaded_nbrb_try_peek((queue_))
#define threaded_queue_dequeue(queue_) threaded_nbrb_dequeue((queue_))
#define threaded_queue_try_dequeue(queue_) threaded_nbrb_try_dequeue((queue_))
#define threaded_queue_dequeue_set(queue_, array_, size_) threaded_nbrb_dequeue_set((queue_),(array_),(size_))
#define threaded_queue_wait_empty(queue_) threaded_nbrb_wait_empty((queue_))
#define threaded_queue_size(queue_) threaded_nbrb_size((queue_))
/*
 * The queue will block (write) if bigger than this.
 * Note that if the key is already bigger it will blocked (write) until
 * the content is emptied by the readers.
 */
#define threaded_queue_set_maxsize(queue_, max_size_) threaded_nbrb_set_maxsize((queue_), (max_size_))

#else

#error THREADED_QUEUE_MODE has not been set to a supported value

#endif

#endif	/* _THREADED_QUEUE_H */
/** @} */

/*----------------------------------------------------------------------------*/

