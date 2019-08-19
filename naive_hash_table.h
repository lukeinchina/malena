#ifndef MALENA_NAIVE_HASH_TABLE_H
#define MALENA_NAIVE_HASH_TABLE_H

#include <stdint.h>

struct NaiveHashTable;
typedef struct NaiveHashTable NaiveHashTable;

struct NaiveHashIter;
typedef struct NaiveHashIter NaiveHashIter;

struct NaiveHashNode;
typedef struct NaiveHashNode NaiveHashNode;

struct NaiveHashNode {
    NaiveHashNode *next;
    uint64_t key;
    void     *val;
};

NaiveHashTable *
ht_create(unsigned long size);

int 
ht_insert(NaiveHashTable *ht, uint64_t key, void *val);

/* search routine */
NaiveHashNode *
ht_lookup(const NaiveHashTable *ht, uint64_t key);
const void *
ht_search(const NaiveHashTable *ht, uint64_t key);

void
ht_clear_items(NaiveHashTable *ht);

NaiveHashTable *
ht_destory(NaiveHashTable *ht);

/* iterator routine */

int ht_iter_isend(const NaiveHashIter *it);
NaiveHashIter * ht_iter_next(NaiveHashIter *it);
NaiveHashIter * ht_iter_begin(NaiveHashIter *it);
NaiveHashIter * ht_iter_new(NaiveHashTable *ht);
NaiveHashIter * ht_iter_del(NaiveHashIter *it);
NaiveHashNode * ht_iter_entry(NaiveHashIter *it);

#endif
