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
/** @defgroup dnsdbzone Zone related functions
 *  @ingroup dnsdb
 *  @brief Internal functions for the database: zoned resource records label.
 *
 *  Internal functions for the database: zoned resource records label.
 *
 * @{
 */

#include <dnscore/format.h>

#include "dnsdb/dictionary.h"
#include "dnsdb/zdb_zone.h"
#include "dnsdb/zdb_zone_label.h"
#include "dnsdb/zdb_record.h"
#include "dnsdb/zdb_dnsname.h"
#include "dnsdb/zdb_utils.h"
#include "dnsdb/zdb_error.h"

/**
 * @brief INTERNAL callback, tests for a match between a label and a node.
 */

static int
zdb_zone_label_zlabel_match(const void *label, const dictionary_node * node)
{
    const zdb_zone_label* zone_label = (const zdb_zone_label*)node;
    return dnslabel_equals(zone_label->name, label);
}

/**
 * @brief INTERNAL callback, creates a new node instance
 */

static dictionary_node *
zdb_zone_label_create(const void *data)
{
    zdb_zone_label* zone_label;

    ZALLOC_OR_DIE(zdb_zone_label*, zone_label, zdb_zone_label,
                  ZDB_ZONELABEL_TAG);

    zone_label->next = NULL;
    dictionary_init(&zone_label->sub);
    zone_label->name = dnslabel_dup(data);
#if ZDB_CACHE_ENABLED!=0
    btree_init(&zone_label->global_resource_record_set);
#endif
    zone_label->zone = NULL;

    return (dictionary_node *)zone_label;
}

/**
 * @brief INTERNAL callback, destroys a node instance and its collections.
 */

static void
zdb_zone_label_destroy_callback(dictionary_node * zone_label_record)
{
    if(zone_label_record == NULL)
    {
        return;
    }

    zdb_zone_label* zone_label = (zdb_zone_label*)zone_label_record;

    /* detach is made by destroy */

    dictionary_destroy(&zone_label->sub, zdb_zone_label_destroy_callback);
#if ZDB_CACHE_ENABLED!=0
    zdb_record_destroy(&zone_label->global_resource_record_set);
#endif
    ZFREE_STRING(zone_label->name);

    zdb_zone_destroy(zone_label->zone);

    ZFREE(zone_label_record, zdb_zone_label);
}

/**
 * @brief Search for the label of a zone in the database
 *
 * Search for the label of a zone in the database
 *
 * @param[in] db the database to explore
 * @param[in] origin the dnsname_vector mapping the label
 * @param[in] zclass the class of the zone we are looking for
 *
 * @return a pointer to the label or NULL if it does not exists in the database.
 *
 */

zdb_zone_label*
zdb_zone_label_find(zdb * db, dnsname_vector* origin, u16 zclass)
{
    zdb_zone_label* zone_label;

#if ZDB_RECORDS_MAX_CLASS==1
    zone_label = db->root[0]; /* the "." zone */
#else
    zone_label = db->root[zclass - 1]; /* the "." zone */
#endif

    dnslabel_stack_reference sections = origin->labels;
    s32 index = origin->size;

    /* look into the sub level */

    while(zone_label != NULL && index >= 0)
    {
        u8* label = sections[index];
        hashcode hash = hash_dnslabel(label);

        zone_label =
                (zdb_zone_label*)dictionary_find(&zone_label->sub, hash, label,
                                                 zdb_zone_label_zlabel_match);

        index--;
    }

    return zone_label;
}

zdb_zone_label*
zdb_zone_label_find_from_name(zdb* db, const char* name, u16 zclass)
{
    dnsname_vector origin;

    u8 dns_name[MAX_DOMAIN_LENGTH];

    if(ISOK(cstr_to_dnsname(dns_name, name)))
    {
        dnsname_to_dnsname_vector(dns_name, &origin);

        return zdb_zone_label_find(db, &origin, zclass);
    }

    return NULL;
}

zdb_zone_label*
zdb_zone_label_find_from_dnsname(zdb* db, const u8* dns_name, u16 zclass)
{
    dnsname_vector origin;

    dnsname_to_dnsname_vector(dns_name, &origin);

    return zdb_zone_label_find(db, &origin, zclass);

    return NULL;
}

/**
 * @brief Destroys a label and its collections.
 *
 * Destroys a label and its collections.
 *
 * @param[in] zone_labelp a pointer to a pointer to the label to destroy.
 *
 */

void
zdb_zone_label_destroy(zdb_zone_label* * zone_labelp)
{
    zassert(zone_labelp != NULL);

    zdb_zone_label* zone_label = *zone_labelp;

    if(zone_label != NULL)
    {
        dictionary_destroy(&zone_label->sub, zdb_zone_label_destroy_callback);
#if ZDB_CACHE_ENABLED!=0
        zdb_record_destroy(&zone_label->global_resource_record_set);
#endif
        zdb_zone_destroy(zone_label->zone);
        ZFREE_STRING(zone_label->name);
        ZFREE(zone_label, zdb_zone_label);
        *zone_labelp = NULL;
    }
}

/**
 * @brief Gets pointers to all the zone labels along the path of a name.
 *
 * Gets pointers to all the zone labels along the path of a name.
 *
 * @param[in] db a pointer to the database
 * @param[in] name a pointer to the dns name
 * @param[in] zclass the class we are looking for
 * @param[in] zone_label_stack a pointer to the stack that will hold the labels pointers
 *
 * @return the top of the stack (-1 = empty)
 */

s32
zdb_zone_label_match(const zdb * db, const dnsname_vector* origin, u16 zclass,
                     zdb_zone_label_pointer_array zone_label_stack)
{
    zdb_zone_label* zone_label;

#if ZDB_RECORDS_MAX_CLASS==1
    zone_label = db->root[0]; /* the "." zone */
#else
    zone_label = db->root[zclass - 1]; /* the "." zone */
#endif

    const_dnslabel_stack_reference sections = origin->labels;
    s32 index = origin->size;

    s32 sp = 0;

    zone_label_stack[0] = zone_label;

    /* look into the sub level */

    while(/*zone_label!=NULL&& */ index >= 0)
    {
        u8* label = sections[index];
        hashcode hash = hash_dnslabel(label);

        zone_label =
                (zdb_zone_label*)dictionary_find(&zone_label->sub, hash, label,
                                                 zdb_zone_label_zlabel_match);

        if(zone_label == NULL)
        {
            break;
        }

        zone_label_stack[++sp] = zone_label;

        index--;
    }

    return sp;
}

/**
 * @brief Retrieve the zone label origin, adds it in the database if needed.
 *
 * Retrieve the zone label origin, adds it in the database if needed.
 *
 * @param[in] db a pointer to the database
 * @param[in] origin a pointer to the dns name
 * @param[in] zclass the class of the zone
 *
 * @return a pointer to the label
 */

zdb_zone_label*
zdb_zone_label_add(zdb * db, dnsname_vector* origin, u16 zclass)
{
    zdb_zone_label* zone_label;

#if ZDB_RECORDS_MAX_CLASS==1
    zone_label = db->root[0]; /* the "." zone */
#else
    zone_label = db->root[zclass - 1]; /* the "." zone */
#endif

    dnslabel_stack_reference sections = origin->labels;
    s32 index = origin->size;

    /* look into the sub level */

    while(index >= 0)
    {
        u8* label = sections[index];
        hashcode hash = hash_dnslabel(label);
        zone_label =
                (zdb_zone_label*)dictionary_add(&zone_label->sub, hash, label,
                                                zdb_zone_label_zlabel_match,
                                                zdb_zone_label_create);

        index--;
    }

    return zone_label;
}

typedef struct zdb_zone_label_delete_process_callback_args
zdb_zone_label_delete_process_callback_args;

struct zdb_zone_label_delete_process_callback_args
{
    dnslabel_stack_reference sections;
    s32 top;
};

/**
 * @brief INTERNAL callback
 */

static ya_result
zdb_zone_label_delete_process_callback(void *a, dictionary_node * node)
{
    zassert(node != NULL);

    zdb_zone_label* zone_label = (zdb_zone_label*)node;

    zdb_zone_label_delete_process_callback_args *args =
            (zdb_zone_label_delete_process_callback_args *)a;

    /*
     * a points to a kind of dnsname and we are going in
     *
     * we go down and down each time calling the dictionnary process for the next level
     *
     * at the last level we return the "delete" code
     *
     * from there, the dictionnary processor will remove the entry
     *
     * at that point the calling dictionnary will know if he has to delete his node or not
     *
     * and so on and so forth ...
     *
     */

    s32 top = args->top;
    u8* label = (u8*)args->sections[top];

    if(!dnslabel_equals(zone_label->name, label))
    {
        return COLLECTION_PROCESS_NEXT;
    }

    /* match */

    if(top > 0)
    {
        /* go to the next level */

        label = args->sections[--args->top];
        hashcode hash = hash_dnslabel(label);

        ya_result err;
        if((err =
                dictionary_process(&zone_label->sub, hash, args,
                                   zdb_zone_label_delete_process_callback)) ==
                COLLECTION_PROCESS_DELETENODE)
        {
            /* check the node for relevance, return "delete" if irrelevant */

            if(ZONE_LABEL_IRRELEVANT(zone_label))
            {
                /* Irrelevant means that only the name remains */

                dictionary_destroy(&zone_label->sub,
                                   zdb_zone_label_destroy_callback);
#if ZDB_CACHE_ENABLED!=0
                zdb_record_destroy(&zone_label->global_resource_record_set);
#endif
                zdb_zone_destroy(zone_label->zone);
                ZFREE_STRING(zone_label->name);
                ZFREE(zone_label, zdb_zone_label);

                return COLLECTION_PROCESS_DELETENODE;
            }

            return COLLECTION_PROCESS_STOP;
        }

        /* or ... stop */

        return err;
    }

    /* NOTE: the 'detach' is made by destroy : do not touch to the "next" field */
    /* NOTE: the freee of the node is made by destroy : do not do it */

    /* dictionary destroy will take every item in the dictionary and
     * iterate through it calling the passed function.
     */

    dictionary_destroy(&zone_label->sub, zdb_zone_label_destroy_callback);
#if ZDB_CACHE_ENABLED!=0
    zdb_record_destroy(&zone_label->global_resource_record_set);
#endif
    zdb_zone_destroy(zone_label->zone);
    ZFREE_STRING(zone_label->name);
    ZFREE(zone_label, zdb_zone_label);

    return COLLECTION_PROCESS_DELETENODE;
}

/**
 * @brief Destroys a zone label and all its collections
 *
 * Destroys a zone label and all its collections
 *
 * @parm[in] db a pointer to the database
 * @parm[in] name a pointer to the name
 * @parm[in] zclass the class of the zone of the label
 *
 * @return an error code
 */

ya_result
zdb_zone_label_delete(zdb * db, dnsname_vector* name, u16 zclass)
{
    zassert(db != NULL && name != NULL && name->size >= 0 && zclass > 0);

    zdb_zone_label* root_label;

#if ZDB_RECORDS_MAX_CLASS==1
    root_label = db->root[0]; /* the "." zone */
#else
    root_label = db->root[zclass - 1]; /* the "." zone */
#endif

    if(root_label == NULL)
    {
        /* has already been destroyed */

        return ZDB_ERROR_NOSUCHCLASS;
    }

    zdb_zone_label_delete_process_callback_args args;
    args.sections = name->labels;
    args.top = name->size;

    hashcode hash = hash_dnslabel(args.sections[args.top]);

    ya_result err;
    if((err =
            dictionary_process(&root_label->sub, hash, &args,
                               zdb_zone_label_delete_process_callback)) ==
            COLLECTION_PROCESS_DELETENODE)
    {
        return COLLECTION_PROCESS_STOP;
    }

    return err;
}

#if ZDB_CACHE_ENABLED!=0

typedef struct zdb_zone_label_delete_record_process_callback_args
zdb_zone_label_delete_record_process_callback_args;

struct zdb_zone_label_delete_record_process_callback_args
{
    dnslabel_stack_reference sections;
    s32 top;
    u16 type;
};

/**
 * @brief INTERNAL callback
 */

static ya_result
zdb_zone_label_delete_record_process_callback(void *a,
                                              dictionary_node * node)
{
    zassert(node != NULL);

    zdb_zone_label* zone_label = (zdb_zone_label*)node;

    zdb_zone_label_delete_record_process_callback_args *args =
            (zdb_zone_label_delete_record_process_callback_args *)a;

    /*
     * a points to a kind of dnsname and we are going in
     *
     * we go down and down each time calling the dictionnary process for the next level
     *
     * at the last level we return the "delete" code
     *
     * from there, the dictionnary processor will remove the entry
     *
     * at that point the calling dictionnary will know if he has to delete his node or not
     *
     * and so on and so forth ...
     *
     */

    s32 top = args->top;
    dnslabel label = (dnslabel)args->sections[top];

    if(!dnslabel_equals(zone_label->name, label))
    {
        return COLLECTION_PROCESS_NEXT;
    }

    /* match */

    if(top > 0)
    {
        /* go to the next level */

        label = args->sections[--args->top];
        hashcode hash = hash_dnslabel(label);

        ya_result err;
        if((err =
                dictionary_process(&zone_label->sub, hash, args,
                                   zdb_zone_label_delete_record_process_callback))
                == COLLECTION_PROCESS_DELETENODE)
        {
            /* check the node for relevance, return "delete" if irrelevant */

            if(ZONE_LABEL_IRRELEVANT(zone_label))
            {
                dictionary_destroy(&zone_label->sub,
                                   zdb_zone_label_destroy_callback);
                zdb_record_destroy(&zone_label->global_resource_record_set);
                zdb_zone_destroy(zone_label->zone);
                ZFREE_STRING(zone_label->name);
                ZFREE(zone_label, zdb_zone_label);

                return COLLECTION_PROCESS_DELETENODE;
            }

            return COLLECTION_PROCESS_STOP;
        }

        /* or ... stop */

        return err;
    }

    /* We are at the right place for the record */

    if(FAIL(zdb_record_delete(&zone_label->global_resource_record_set, args->type))) /* CACHE: No FeedBack  */
    {
        return COLLECTION_PROCESS_RETURNERROR;
    }

    if(ZONE_LABEL_RELEVANT(zone_label))
    {
        return COLLECTION_PROCESS_STOP;
    }

    /* NOTE: the 'detach' is made by destroy : do not touch to the "next" field */
    /* NOTE: the freee of the node is made by destroy : do not do it */

    /* dictionary destroy will take every item in the dictionary and
     * iterate through it calling the passed function.
     */

    dictionary_destroy(&zone_label->sub, zdb_zone_label_destroy_callback);
    zdb_record_destroy(&zone_label->global_resource_record_set);
    zdb_zone_destroy(zone_label->zone);
    ZFREE_STRING(zone_label->name);
    ZFREE(zone_label, zdb_zone_label);

    return COLLECTION_PROCESS_DELETENODE;
}

/**
 * @brief Destroys all records of a given type for a zone label (cache)
 *
 * Destroys all records of a given type for a zone label (cache)
 *
 * @parm[in] db a pointer to the database
 * @parm[in] name a pointer to the name
 * @parm[in] zclass the class of the zone of the label
 * @parm[in] type the type of the records to delete
 *
 * @return an error code
 */

ya_result
zdb_zone_label_delete_record(zdb * db, dnsname_vector* origin, u16 zclass,
                             u16 type)
{
    zassert(db != NULL && origin != NULL && origin->size >= 0);

    zdb_zone_label* root_label;

#if ZDB_RECORDS_MAX_CLASS==1
    root_label = db->root[0]; /* the "." zone */
#else
    root_label = db->root[zclass - 1]; /* the "." zone */
#endif

    if(root_label == NULL)
    {
        /* has already been destroyed */

        return ZDB_ERROR_NOSUCHCLASS;
    }

    zdb_zone_label_delete_record_process_callback_args args;
    args.sections = origin->labels;
    args.top = origin->size;
    args.type = type;

    hashcode hash = hash_dnslabel(args.sections[args.top]);

    ya_result err;
    if((err =
            dictionary_process(&root_label->sub, hash, &args,
                               zdb_zone_label_delete_record_process_callback)) ==
            COLLECTION_PROCESS_DELETENODE)
    {
        if(ZONE_LABEL_IRRELEVANT(root_label))
        {
            dictionary_destroy(&root_label->sub,
                               zdb_zone_label_destroy_callback);
            zdb_record_destroy(&root_label->global_resource_record_set);
            ZFREE_STRING(root_label->name);
            ZFREE(root_label, zdb_zone_label);

#if ZDB_RECORDS_MAX_CLASS==1
            db->root[0] = NULL;
#else
            db->root[zclass - 1] = NULL;
#endif

            return COLLECTION_PROCESS_DELETENODE;
        }

        return COLLECTION_PROCESS_STOP;
    }

    return err;
}

typedef struct zdb_zone_label_delete_record_exact_process_callback_args
zdb_zone_label_delete_record_exact_process_callback_args;

struct zdb_zone_label_delete_record_exact_process_callback_args
{
    dnslabel_stack_reference sections;
    s32 top;
    u16 type;
    zdb_ttlrdata* ttlrdata;
};

/**
 * @brief INTERNAL callback
 */

static ya_result
zdb_zone_label_delete_record_exact_process_callback(void *a,
                                                    dictionary_node * node)
{
    zassert(node != NULL);

    zdb_zone_label* zone_label = (zdb_zone_label*)node;

    zdb_zone_label_delete_record_exact_process_callback_args *args =
            (zdb_zone_label_delete_record_exact_process_callback_args *)a;

    /*
     * a points to a kind of dnsname and we are going in
     *
     * we go down and down each time calling the dictionnary process for the next level
     *
     * at the last level we return the "delete" code
     *
     * from there, the dictionnary processor will remove the entry
     *
     * at that point the calling dictionnary will know if he has to delete his node or not
     *
     * and so on and so forth ...
     *
     */

    s32 top = args->top;
    dnslabel label = (dnslabel)args->sections[top];

    if(!dnslabel_equals(zone_label->name, label))
    {
        return COLLECTION_PROCESS_NEXT;
    }

    /* match */

    if(top > 0)
    {
        /* go to the next level */

        label = args->sections[--args->top];
        hashcode hash = hash_dnslabel(label);

        ya_result err;
        if((err =
                dictionary_process(&zone_label->sub, hash, args,
                                   zdb_zone_label_delete_record_exact_process_callback))
                == COLLECTION_PROCESS_DELETENODE)
        {
            /* check the node for relevance, return "delete" if irrelevant */

            if(ZONE_LABEL_IRRELEVANT(zone_label))
            {
                /* Irrelevant means that only the name remains
                 * Still, it's not because a collection is empty that it does not uses memory.
                 */

                dictionary_destroy(&zone_label->sub,
                                   zdb_zone_label_destroy_callback);
                zdb_record_destroy(&zone_label->global_resource_record_set);
                zdb_zone_destroy(zone_label->zone);
                ZFREE_STRING(zone_label->name);
                ZFREE(zone_label, zdb_zone_label);

                return COLLECTION_PROCESS_DELETENODE;
            }

            return COLLECTION_PROCESS_STOP;
        }

        /* or ... stop */

        return err;
    }

    /* We are at the right place for the record */

    if(FAIL(zdb_record_delete_exact(&zone_label->global_resource_record_set, args->type, args->ttlrdata))) /* CACHE: No FeedBack */
    {
        return COLLECTION_PROCESS_RETURNERROR;
    }

    if(ZONE_LABEL_RELEVANT(zone_label))
    {
        return COLLECTION_PROCESS_STOP;
    }

    /* NOTE: the 'detach' is made by destroy : do not touch to the "next" field */
    /* NOTE: the freee of the node is made by destroy : do not do it */

    /* dictionary destroy will take every item in the dictionary and
     * iterate through it calling the passed function.
     */

    dictionary_destroy(&zone_label->sub, zdb_zone_label_destroy_callback);
    zdb_record_destroy(&zone_label->global_resource_record_set);
    zdb_zone_destroy(zone_label->zone);
    ZFREE_STRING(zone_label->name);
    ZFREE(zone_label, zdb_zone_label);

    return COLLECTION_PROCESS_DELETENODE;
}

/**
 * @brief Destroys a record matching of a given type, ttl and rdata for a zone label (cache)
 *
 * Destroys a record matching of a given type, ttl and rdata for a zone label (cache)
 *
 * @parm[in] db a pointer to the database
 * @parm[in] name a pointer to the name
 * @parm[in] zclass the class of the zone of the label
 * @parm[in] type the type of the records to delete
 * @parm[in] ttlrdata the ttl and rdata to match
 *
 * @return an error code
 */

ya_result
zdb_zone_label_delete_record_exact(zdb * db, dnsname_vector* origin,
                                   u16 zclass, u16 type,
                                   zdb_ttlrdata* ttlrdata)
{
    zassert(db != NULL && origin != NULL && origin->size >= 0);

    zdb_zone_label* root_label;

#if ZDB_RECORDS_MAX_CLASS==1
    root_label = db->root[0]; /* the "." zone */
#else
    root_label = db->root[zclass - 1]; /* the "." zone */
#endif

    if(root_label == NULL)
    {
        /* has already been destroyed */

        return ZDB_ERROR_NOSUCHCLASS;
    }

    zdb_zone_label_delete_record_exact_process_callback_args args;
    args.sections = origin->labels;
    args.top = origin->size;
    args.type = type;
    args.ttlrdata = ttlrdata;

    hashcode hash = hash_dnslabel(args.sections[args.top]);

    ya_result err;
    if((err =
            dictionary_process(&root_label->sub, hash, &args,
                               zdb_zone_label_delete_record_exact_process_callback))
            == COLLECTION_PROCESS_DELETENODE)
    {
        if(ZONE_LABEL_IRRELEVANT(root_label))
        {
            dictionary_destroy(&root_label->sub,
                               zdb_zone_label_destroy_callback);
            zdb_record_destroy(&root_label->global_resource_record_set);
            ZFREE_STRING(root_label->name);
            ZFREE(root_label, zdb_zone_label);

#if ZDB_RECORDS_MAX_CLASS==1
            db->root[0] = NULL;
#else
            db->root[zclass - 1] = NULL;
#endif

            return COLLECTION_PROCESS_DELETENODE;
        }

        return COLLECTION_PROCESS_STOP;
    }

    return err;
}

#endif

#ifndef NDEBUG

/**
 * DEBUG
 */

void
zdb_zone_label_print_indented(zdb_zone_label* zone_label, int indented)
{
    if(zone_label == NULL)
    {
        formatln("%tg: NULL", indented);
        return;
    }

    if(zone_label->zone != NULL)
    {
        zdb_zone_print_indented(zone_label->zone, indented + 1);
    }

    if(zone_label->name != NULL)
    {
        formatln("%tg: '%{dnslabel}'", indented, zone_label->name);
    }
    else
    {
        formatln("%tg: WRONG", indented);
    }

#if ZDB_CACHE_ENABLED!=0
    zdb_record_print_indented(zone_label->global_resource_record_set, indented);
#endif

    dictionary_iterator iter;
    dictionary_iterator_init(&zone_label->sub, &iter);

    while(dictionary_iterator_hasnext(&iter))
    {
        zdb_zone_label* *sub_labelp = (zdb_zone_label* *)dictionary_iterator_next(&iter);

        zdb_zone_label_print_indented(*sub_labelp, indented + 1);
    }
}

void
zdb_zone_label_print(zdb_zone_label* zone_label)
{
    zdb_zone_label_print_indented(zone_label, 0);
}

#endif

/** @} */
