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
#include <stdlib.h>
#include <unistd.h>

#include "dnscore/threaded_ringbuffer_cw.h"

#define THREADED_QUEUE_TAG	    0x455545555154	/* TQUEUE */

/*
 * The maximum number of nodes I should ever require is the queue size + 1
 *
 * A good way to handle this is :
 *
 * Node allocator, node destroyer
 *
 * But the node allocator only allocates a node if none are free
 * And the node destroyer does not release a node but queue it instead.
 *
 * If I enforce the requirement that the allocation/destruction is done
 * by a single thread I could gain some cpu cycles.
 *
 * This reasoning is also valid for the thread_pool_task
 *
 */

typedef struct threaded_ringbuffer_cw_node threaded_ringbuffer_cw_node;


struct threaded_ringbuffer_cw_node
{
    threaded_ringbuffer_cw_node *next;
    threaded_ringbuffer_cw_node *prev;
    
    void* data;
};

/*
 * Note:
 *
 * If a pthread_mutex_init fails, it's because of a resource, memory or rights issue.
 * So the application will fail soon enough.
 * I still should check this and exit.
 *
 * pthread_mutex_lock will fail only if the current thread aleady owns the mutex
 *
 * pthread_mutex_unlock will fail only if the current thread does not owns the mutex
 *
 */

void
threaded_ringbuffer_cw_init(threaded_ringbuffer_cw *queue, int max_size)
{
    MALLOC_OR_DIE(void**, queue->buffer, sizeof (void*) * max_size, THREADED_QUEUE_TAG);
    queue->buffer_limit = &queue->buffer[max_size];
    queue->write_slot = queue->buffer;
    queue->read_slot = queue->buffer;

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond_read, NULL);
    pthread_cond_init(&queue->cond_write, NULL);

    queue->max_size = max_size;
    queue->size = 0;
}

void
threaded_ringbuffer_cw_finalize(threaded_ringbuffer_cw *queue)
{
    threaded_ringbuffer_cw_node* node;

    /**
     * If the queue is not empty : too bad !
     *
     * It's the responsibility of the caller to ensure the queue and  set of listeners is empty.
     */

    free(queue->buffer);
    queue->buffer = NULL;

    pthread_cond_destroy(&queue->cond_read);
    pthread_cond_destroy(&queue->cond_write);
    pthread_mutex_destroy(&queue->mutex);
}

void
threaded_ringbuffer_cw_enqueue(threaded_ringbuffer_cw* queue, void* constant_pointer)
{
    /*
     * Ensure I'm allowed to work on queue (only one working on it)
     */

    pthread_mutex_lock(&queue->mutex);
    while( queue->size >= queue->max_size )
    {
        pthread_cond_wait(&queue->cond_write,&queue->mutex);
    }

    /*
     * Set the data to the write position,
     * and move the write position to the next slot
     *
     */

    /**
     * @note: "if(overflow) reset" is (much) faster than MOD(limit)
     */

    *queue->write_slot++ = constant_pointer;

    if(queue->write_slot == queue->buffer_limit)
    {
        queue->write_slot = queue->buffer;
    }

    queue->size++;

    /*
     * We are done here, we can always signal the readers
     */

    pthread_cond_broadcast(&queue->cond_read);
    pthread_mutex_unlock(&queue->mutex);
}

bool
threaded_ringbuffer_cw_try_enqueue(threaded_ringbuffer_cw* queue, void* constant_pointer)
{
    /*
     * Ensure I'm allowed to work on queue (only one working on it)
     */

    if(pthread_mutex_trylock(&queue->mutex) != 0)
    {
        return FALSE;
    }

    if( queue->size >= queue->max_size )
    {
        return FALSE;
    }

    /*
     * Set the data to the write position,
     * and move the write position to the next slot
     *
     */

    /**
     * @note: "if(overflow) reset" is (much) faster than MOD(limit)
     */

    *queue->write_slot++ = constant_pointer;

    if(queue->write_slot == queue->buffer_limit)
    {
	queue->write_slot = queue->buffer;
    }

    queue->size++;

    /*
     * We are done here, we can always signal the readers
     */

    pthread_cond_broadcast(&queue->cond_read);
    pthread_mutex_unlock(&queue->mutex);

    return TRUE;
}

void*
threaded_ringbuffer_cw_peek(threaded_ringbuffer_cw *queue)
{
    /*
     * Ensure I'm allowed to work on queue (only one working on it)
     */

    pthread_mutex_lock(&queue->mutex);

    while( queue->size == 0 )
    {
        pthread_cond_wait(&queue->cond_read,&queue->mutex);
    }

    /*
     * Get the data from the read position,
     * and move the read position to the next slot
     *
     */

    void* data = *queue->read_slot;
   
    /*
     * We are done here.
     */

    pthread_mutex_unlock(&queue->mutex);

    return data;
}

void*
threaded_ringbuffer_cw_try_peek(threaded_ringbuffer_cw *queue)
{
    pthread_mutex_lock(&queue->mutex);

    if( queue->size == 0 )
    {
        pthread_mutex_unlock(&queue->mutex);

        return NULL;
    }

    /*
     * Get the data from the read position,
     * and move the read position to the next slot
     *
     */

    void* data = *queue->read_slot;

    /*
     * We are done here.
     */

    pthread_mutex_unlock(&queue->mutex);

    return data;
}


void*
threaded_ringbuffer_cw_dequeue(threaded_ringbuffer_cw *queue)
{
    /*
     * Ensure I'm allowed to work on queue (only one working on it)
     */

    pthread_mutex_lock(&queue->mutex);

    while( queue->size == 0 )
    {
        pthread_cond_wait(&queue->cond_read,&queue->mutex);
    }

    /*
     * Get the data from the read position,
     * and move the read position to the next slot
     *
     */

    void* data = *queue->read_slot++;
    if(queue->read_slot == queue->buffer_limit)
    {
        queue->read_slot = queue->buffer;
    }

    if(queue->size-- == queue->max_size) /* enqueue has just been locked  -> unlock */
    {
        /*
         * The queue is full : the queuers are waiting.
         * Since we will are removing something, we car free (one of) them.
         * (They will however still be locked until the queue mutex is released)
         */

        pthread_cond_broadcast(&queue->cond_write);
    }

    /*
     * We are done here.
     */

    pthread_mutex_unlock(&queue->mutex);

    return data;
}

void*
threaded_ringbuffer_cw_try_dequeue(threaded_ringbuffer_cw *queue)
{
    pthread_mutex_lock(&queue->mutex);

    if( queue->size == 0 )
    {
        pthread_mutex_unlock(&queue->mutex);

        return NULL;
    }

    /*
     * Get the data from the read position,
     * and move the read position to the next slot
     *
     */

    void* data = *queue->read_slot++;
    if(queue->read_slot == queue->buffer_limit)
    {
	queue->read_slot = queue->buffer;
    }

    if(queue->size-- == queue->max_size) /* enqueue has just been locked  -> unlock */
    {
	/*
	 * The queue is full : the queuers are waiting.
	 * Since we will are removing something, we car free (one of) them.
	 * (They will however still be locked until the queue mutex is released)
	 */

	pthread_cond_broadcast(&queue->cond_write);
    }

    /*
     * We are done here.
     */

    pthread_mutex_unlock(&queue->mutex);

    return data;
}

u32   threaded_ringbuffer_cw_dequeue_set(threaded_ringbuffer_cw* queue, void** array, u32 array_size)
{
    /*
     * Ensure I'm allowed to work on queue (only one working on it)
     */

    pthread_mutex_lock(&queue->mutex);

    while( queue->size == 0 )
    {
        pthread_cond_wait(&queue->cond_read,&queue->mutex);
    }

    /*
     * Get up to array_size times the data from the read position,
     * and move the read position to the next slot
     *
     */

    bool unlock_enqueue = queue->size == queue->max_size; /* enqueue has just been locked -> schedule unlock */

    u32 loops = MIN(queue->size, array_size);		  /* The amount we will be able to extract */
    
    void** limit = &array[loops];			  /* compute the limit so we only have one increment and one compare */

    while(array < limit)
    {
	void* data = *queue->read_slot++;
	*array++ = data;

	if(queue->read_slot == queue->buffer_limit)
	{
	    queue->read_slot = queue->buffer;
	}

	if(data == NULL)				    /* Break if a terminator is found*/
	{
	    loops -= limit-array;
	    break;
	}
    }

    queue->size -= loops;				  /* adjust the size */

    if(unlock_enqueue) /* enqueue has just been locked -> unlock */
    {
	/*
	 * The queue is full : the queuers are waiting.
	 * Since we will are removing something, we car free (one of) them.
	 * (They will however still be locked until the queue mutex is released)
	 */

	pthread_cond_broadcast(&queue->cond_write);
    }   

    /*
     * We are done here.
     */

    pthread_mutex_unlock(&queue->mutex);

    return loops;	    /* Return the amount we got from the queue */
}

void
threaded_ringbuffer_cw_wait_empty(threaded_ringbuffer_cw *queue)
{
    int size;

    for(;;)
    {
        pthread_mutex_lock(&queue->mutex);

        size = queue->size;

        pthread_mutex_unlock(&queue->mutex);

        if(size == 0)
        {
            break;
        }

        usleep(1000);
    }
}

int
threaded_ringbuffer_cw_size(threaded_ringbuffer_cw *queue)
{
    int size;

    pthread_mutex_lock(&queue->mutex);

    size = queue->size;

    pthread_mutex_unlock(&queue->mutex);

    return size;

}

ya_result
threaded_ringbuffer_cw_set_maxsize(threaded_ringbuffer_cw *queue, int max_size)
{
    ya_result ret = ERROR;

    pthread_mutex_lock(&queue->mutex);

    if(max_size >= queue->size)
    {
        void** tmp;
        MALLOC_OR_DIE(void**, tmp, sizeof (void*) * max_size, THREADED_QUEUE_TAG);

        /*
         * Copy from the read to the write position
         */

        void** p = tmp;
        u32 count = queue->size;

        while(count-- > 0)
        {
            *p = *queue->read_slot++;

            if(queue->read_slot == queue->buffer_limit)
            {
            queue->read_slot = queue->buffer;
            }
        }

        /*
         * At this point ...
         *
         * tmp is the new "read"
         * p is the new "write", but it could be at the limit
         *
         */

        free(queue->buffer);
        queue->buffer = tmp;
        queue->buffer_limit = &tmp[max_size];
        queue->read_slot = tmp;

        if(p == queue->buffer_limit)
        {
            p = tmp;
        }

        queue->write_slot = p;

        queue->max_size = max_size;
    }

    pthread_mutex_unlock(&queue->mutex);

    return ret;
}

/** @} */

/*----------------------------------------------------------------------------*/

