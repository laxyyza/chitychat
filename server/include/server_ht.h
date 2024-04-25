/*
 * Server Hash Table
 */

#ifndef _SERVER_HT_H_
#define _SERVER_HT_H_

#include "common.h"
#include "server_tm.h"

typedef void (*ght_free_t)(void* data);

typedef struct ght_bucket
{
    u64     key;
    void*   data;
    i8      inheap;

    struct ght_bucket* next;
} ght_bucket_t;

/*
 * Server Generic Hash Table - (GHT; SERVER_GHT)
 *
 * Featurs:
 *  - Dynamic sizing based on load factor, with adjustable max & min thresholds.
 *  - Thread-Safe.
 *  - Automatic memory management (if `server_ght_t::free` is provided).
 *  - Generic Types.
 *
 * NOTE:
 *  If `server_ght_t::free` is provided, GHT will assume ownership of elements.
 */
typedef struct
{
    /* Array */
    ght_bucket_t*   table;
    size_t          size;
    size_t          min_size;
    size_t          count;

    pthread_mutex_t mutex;
    ght_free_t      free;

    /* Load Factor & min/max thresholds */
    f32             load;
    f32             max_load;
    f32             min_load;
} server_ght_t;

/* return: false if failed */
bool    server_ght_init(server_ght_t* ht, 
                        size_t initial_size, 
                        ght_free_t free_callback);

/* Hash String */
u64     server_ght_hashstr(const char* str);

/* return: false if `key` already exists in table. */
bool    server_ght_insert(server_ght_t* ht, u64 key, void* data);

/* return: NULL if not found */
void*   server_ght_get(server_ght_t* ht, u64 key);

/* return: false if not found. */
bool    server_ght_del(server_ght_t* ht, u64 key);

/* Delete all elements; clear table. */
void    server_ght_clear(server_ght_t* ht);

/* Delete all elements, mutex and table array. */
void    server_ght_destroy(server_ght_t* ht);

/* Loop each element in hash table. */
#define GHT_FOREACH(item, ht, code_block)\
    for (size_t i = 0; i < ht->size; i++)\
    {\
        ght_bucket_t* _bucket = ht->table + i;\
        while (_bucket && _bucket->data)\
        {\
            item = _bucket->data;\
            code_block\
            _bucket = _bucket->next;\
        }\
    }

#endif // _SERVER_HT_H_
