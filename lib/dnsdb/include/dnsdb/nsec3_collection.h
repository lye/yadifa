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
 *  This is the collection that holds the NSEC3 chain for one NSEC3PARAM
 *
 * @{
 */

#ifndef _NSEC3_COLLECTION_H
#define	_NSEC3_COLLECTION_H

#include <dnsdb/nsec3_types.h>

#ifdef	__cplusplus
extern "C"
{
#endif

/*
 * A digest is stored prefixed with its length ([1;255])
 */

/*
 * A structure to hold both children with direct access
 */

typedef struct nsec3_node nsec3_node;


struct nsec3_children
{
    struct nsec3_node* left;
    struct nsec3_node* right;
};

/*
 * An union to have access to the children with direct or indexed access
 */

typedef union nsec3_children_union nsec3_children_union;

union nsec3_children_union
{
    struct nsec3_children lr;
    struct nsec3_node * child[2];
};

typedef union nsec3_label_pointer_array nsec3_label_pointer_array;

union nsec3_label_pointer_array
{
    zdb_rr_label* owner;
    zdb_rr_label** owners;
};

/*
 * The node structure CANNOT have a varying size on a given collection
 * This means that the digest size is a constant in the whole tree
 */

struct nsec3_node
{
    union nsec3_children_union children;
    /**/
    struct nsec3_node* parent;
    /**/

    /* 64 bits aligned */
    s8 balance;

    /* PAYLOAD BEYOND THIS POINT */

    u8 flags; /* opt-out */
    u16 type_bit_maps_size;
    u16 rc; /* label RC */
    u16 sc; /* *.label RC */

    /* 64 bits aligned */

    zdb_packed_ttlrdata* rrsig;
    /**/

    nsec3_label_pointer_array label;
    nsec3_label_pointer_array star_label;

    u8 *type_bit_maps; /* MUST be a ptr */
    
    u8 digest[1];
    /* 7*4	7*8
     * 3*2      3*2
     * 1*1      1*1
     *
     * 35       63
     *
     * +21
     *
     *56 (56)   84 (88)
     *
     * For the 3M records of EU:
     *
     * 168MB    240MB
     *
     * To this, 2+1 ptrs must be added
     * by record
     *
     * The remaining overhead happens
     * if there are many references
     * for the same label, this should
     * be fairly negligible.
     *
     * 36MB    72MB
     *
     * =>
     *
     * 204MB   312MB
     *
     */
};


#define NSEC3_NODE_SIZE_FOR_DIGEST(node,digest) ((sizeof(nsec3_node)-1)+digest[0])

#define NSEC3_NODE_DIGEST_SIZE(node) (node->digest[0])
#define NSEC3_NODE_DIGEST_PTR(node) (&node->digest[1])

#define NSEC3_NODE_SIZE(node) ((sizeof(nsec3_node)-1)+NSEC3_NODE_DIGEST_SIZE(node))

/*
 * AVL definition part begins here
 */

/*
 * The maximum depth of a tree.
 * 40 is enough for storing 433494436 items (worst case)
 *
 * Depth 0 is one node.
 *
 * Worst case : N is enough for sum[n = 0,N](Fn) where F is Fibonacci
 * Best case : N is enough for (2^(N+1))-1
 */
#define AVL_MAX_DEPTH   40 /* 64 */

/*
 * The previx that will be put in front of each function name
 */
#define AVL_PREFIX	nsec3_

/*
 * The type that hold the node
 */
#define AVL_NODE_TYPE   nsec3_node

/*
 * The type that hold the tree (should be AVL_NODE_TYPE*)
 */
#define AVL_TREE_TYPE   AVL_NODE_TYPE*

/*
 * The type that hold the tree (should be AVL_NODE_TYPE*)
 */
#define AVL_CONST_TREE_TYPE AVL_NODE_TYPE* const

/*
 * How to find the root in the tree
 */
#define AVL_TREE_ROOT(__tree__) (*(__tree__))

/*
 * The type used for comparing the nodes.
 */
#define AVL_REFERENCE_TYPE u8*

/*
 * The node has got a pointer to its parent
 *
 * 0   : disable
 * !=0 : enable
 */
#define AVL_HAS_PARENT_POINTER 1

#ifdef	__cplusplus
}
#endif

#include <dnscore/avl.h.inc>

#ifdef	__cplusplus
extern "C"
{
#endif

AVL_NODE_TYPE* AVL_PREFIXED(avl_find_interval_start)(AVL_TREE_TYPE* tree, AVL_REFERENCE_TYPE obj_hash);

/*
 * I recommand setting a define to identify the C part of the template
 * So it can be used to undefine what is not required anymore for every
 * C file but that one.
 *
 */

#ifndef _NSEC3_COLLECTION_C

#undef AVL_MAX_DEPTH
#undef AVL_PREFIX
#undef AVL_NODE_TYPE
#undef AVL_TREE_TYPE
#undef AVL_CONST_TREE_TYPE
#undef AVL_TREE_ROOT
#undef AVL_REFERENCE_TYPE
#undef AVL_HAS_PARENT_POINTER

#undef _AVL_H_INC

#endif	/* _NSEC3_COLLECTION_C */

#ifdef	__cplusplus
}
#endif

/*
 * AVL definition part ends here
 */

#endif	/* _NSEC3_COLLECTION_H */

/** @} */

/*----------------------------------------------------------------------------*/

