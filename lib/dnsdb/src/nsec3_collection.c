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
/** @defgroup nsec3 NSEC3 functions
 *  @ingroup dnsdbdnssec
 *  @brief
 *
 *
 *
 * @{
 */
/*------------------------------------------------------------------------------
 *
 * USE INCLUDES */
#include <stdio.h>
#include <stdlib.h>

#include <dnscore/logger.h>

#define _NSEC3_COLLECTION_C

#define MODULE_MSG_HANDLE g_dnssec_logger
extern logger_handle *g_dnssec_logger;

#define DUMPER_FUNCTION(...) logger_handle_msg(MODULE_MSG_HANDLE,MSG_DEBUG,__VA_ARGS__)

//#define DEBUG_LEVEL 9
//#define DEBUG_DUMP

#include <dnscore/dnscore.h>
#include "dnsdb/nsec3_collection.h"

/*
 * The following macros are defining relevant fields in the node
 */

/*
 * Access to the field that points to the left child
 *
 */
#define AVL_LEFT_CHILD(node) ((node)->children.lr.left)
/*
 * Access to the field that points to the right child
 */
#define AVL_RIGHT_CHILD(node) ((node)->children.lr.right)
/*
 * Access to the field that points to one of the children (0: left, 1: right)
 */
#define AVL_CHILD(node,id) ((node)->children.child[(id)])
/*
 * OPTIONAL : Access to the field that points the parent of the node.
 *
 * This field is optional but is mandatory if AVL_HAS_PARENT_POINTER is not 0
 */
#define AVL_PARENT(node) ((node)->parent)
/*
 * Access to the field that keeps the balance (a signed byte)
 */
#define AVL_BALANCE(node) ((node)->balance)
/*
 * The type used for comparing the nodes.
 */
#define AVL_REFERENCE_TYPE u8*
/*
 *
 */

#define AVL_REFERENCE_FORMAT_STRING "%{digest32h}"
#define AVL_REFERENCE_FORMAT(reference) reference

/*
 * Two helpers macro, nothing to do with AVL
 */
#define NODE_SIZE(node) (sizeof(AVL_NODE_TYPE) + (node)->digest[0])
#define NODE_PAYLOAD_SIZE(node) (NODE_SIZE(node)-sizeof(nsec3_children_union)-1)
/*
 * A macro to initialize a node and setting the reference
 */
#define AVL_INIT_NODE(node,reference) MEMCOPY((node)->digest,&(reference)[0],(reference)[0]+1)
/*
 * A macro to allocate a new node
 */
#define AVL_ALLOC_NODE(node,reference)				\
	zassert((reference)[0]!=0);					    \
	ZALLOC_ARRAY_OR_DIE(AVL_NODE_TYPE*, node, (sizeof(AVL_NODE_TYPE)+(reference)[0]), AVL_NODE_TAG); \
	ZEROMEMORY(node,sizeof(AVL_NODE_TYPE)+(reference)[0])

/*
 * A macro to free a node allocated by ALLOC_NODE
 */

static void
nsec3_free_node(nsec3_zone_item* node)
{
    /*
     * This assert is wrong because this is actually the payload that has just overwritten our node
     * assert(node->rc == 0 && node->sc == 0 && node->label.owners == NULL && node->star_label.owners == NULL & node->type_bit_maps == NULL);
     */
    ZFREE_ARRAY(node, NSEC3_NODE_SIZE(node));
}

#define AVL_FREE_NODE(node) nsec3_free_node(node)
/*
 * A macro to print the node
 */
#define AVL_DUMP_NODE(node) format("node@%p",(node));
/*
 * A macro that returns the reference field of the node.
 * It must be of type REFERENCE_TYPE
 */
#define AVL_REFERENCE(node) (node)->digest
/*
 * A macro to compare two references
 * Returns TRUE if and only if the references are equal.
 */
#define AVL_ISEQUAL(reference_a,reference_b) (memcmp(&(reference_a)[1],&(reference_b)[1],(reference_a)[0])==0)
/*
 * A macro to compare two references
 * Returns TRUE if and only if the first one is bigger than the second one.
 */
#define AVL_ISBIGGER(reference_a,reference_b) (memcmp(&(reference_a)[1],&(reference_b)[1],(reference_a)[0])>0)
/*
 * Copies the payload of a node
 * It MUST NOT copy the "proprietary" node fields : children, parent, balance
 */
#define AVL_COPY_PAYLOAD(node_trg,node_src) MEMCOPY(&(node_trg)->flags,&(node_src)->flags,NODE_PAYLOAD_SIZE(node))
/*
 * A macro to preprocess a node before it is preprocessed for a delete (detach)
 * If there was anything to do BEFORE deleting a node, we would do it here
 * After this macro is exectuted, the node
 * _ is detached, then deleted with FREE_NODE
 * _ has got its content overwritten by the one of another node, then the other
 *   node is deleted with FREE_NODE
 */
#define AVL_NODE_DELETE_CALLBACK(node)

#include <dnscore/avl.c.inc>

AVL_NODE_TYPE*
AVL_PREFIXED(avl_find_interval_start)(AVL_TREE_TYPE* root, AVL_REFERENCE_TYPE obj_hash)
{
    AVL_NODE_TYPE* node = *root;
    AVL_NODE_TYPE* lower_bound = NULL;
    AVL_REFERENCE_TYPE h;
    
    zassert(node != NULL);

    /* This is one of the parts I could try to optimize
     * I've checked the assembly, and it sucks ...
     */

    /* Both the double-test while/ternary and the current one
     * are producing the same assembly code.
     */
    
    /**
     * Get a key that 
     */

    while(node != NULL)
    {
        h = AVL_REFERENCE(node);

        /*
         * [0] is the length of the obj_hash
         *
         * The obj_hashs starts at [1]
         *
         */

#ifndef NDEBUG
        if(h[0] != obj_hash[0])
        {
            DIE_MSG("NSEC3 corrupted NSEC3 node");
        }
#endif

        int cmp = memcmp(&obj_hash[1], &h[1], h[0]);

        /* equals */
        if(cmp == 0)
        {
            return node;
        }

        /* bigger */
        if(cmp > 0)
        {
            lower_bound = node;
            node = AVL_CHILD(node, DIR_RIGHT);            
        }
        else
        {
            node = AVL_CHILD(node, DIR_LEFT);
        }
    }
    
    if(lower_bound == NULL)
    {
        lower_bound = *root;
        
        zassert(lower_bound != NULL);
        
        while((node = AVL_CHILD(lower_bound, DIR_RIGHT)) != NULL)
        {
            lower_bound = node;
        }
    }
    
    return lower_bound;
}

/** @} */

/*----------------------------------------------------------------------------*/

