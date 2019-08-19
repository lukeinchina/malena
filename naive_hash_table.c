#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "naive_hash_table.h"

const static unsigned long prime_candidates[] = {
    61,   83,
    113,      151,      211,      281,      379,      509,      683,      911,
    1217,     1627,     2179,     2909,     3881,     6907,     9209,
    12281,    16381,    21841,    29123,    38833,    51787,    69061,    92083,
    122777,   163729,   218357,   291143,   388211,   517619,   690163,   999983,
    1226959,  1635947,  2181271,  2908361,  3877817,  5170427,  6893911,  9191891,
    12255871, 16341163, 21788233, 29050993, 38734667, 51646229, 68861641, 91815541,
    1e9+7,    1e9+9,
    122420729,163227661,217636919,290182597,386910137,515880193,687840301,917120411,
    1222827239,1610612741,3221225473ul,4294967291ul
};

struct NaiveHashTable {
    unsigned long buckets_size;
    unsigned long item_count;
    NaiveHashNode **buckets;
};

struct NaiveHashIter {
    NaiveHashTable *ht;
    unsigned long  bucket_index;
    NaiveHashNode  *node_ptr;
};

/*---------------------------------------------------------------------------*/

NaiveHashTable *
ht_create(unsigned long size) {
    long idx = 0;
    NaiveHashTable *ht = NULL;
    long array_size = sizeof(prime_candidates) / sizeof(prime_candidates[0]);
    for  (; idx < array_size && prime_candidates[idx] < size; idx++) {
    }

    ht = malloc(sizeof(NaiveHashTable));
    assert(NULL != ht);
    ht->buckets_size = prime_candidates[idx];
    ht->item_count = 0LU;

    ht->buckets = (NaiveHashNode **)calloc(ht->buckets_size, \
                          sizeof(NaiveHashNode));
    assert(NULL != ht->buckets);

    return ht;
}

void
ht_clear_items(NaiveHashTable *ht) {
    unsigned long        idx  = 0;
    NaiveHashNode **pp = NULL;
    NaiveHashNode *p   = NULL;

    for (; idx < ht->buckets_size; idx++) {
        pp = ht->buckets + idx;
        while (NULL != *pp) {
            p   = *pp;
            *pp = p->next;
            free(p);
        }
    }
    ht->item_count = 0LU;
    return;
}

NaiveHashTable *
ht_destory(NaiveHashTable *ht) {
    ht_clear_items(ht);
    ht->buckets_size = 0;
    free(ht->buckets);
    free(ht);
    return NULL;
}

/*
 * @brief: 插入数据。如果key已经存在，则不会重复插入，并且返回非0.
 *         ps，在同一个桶内，采用的是尾部插入。
 * @return: succ:0; other:not 0
 */
int
ht_insert(NaiveHashTable *ht, uint64_t key, void *val) {
    NaiveHashNode **pp = ht->buckets + (key % ht->buckets_size);

    while (NULL != *pp && key != (*pp)->key) {
        pp = &(*pp)->next;
    }
    if (NULL != *pp) {
        return -1;
    }

    *pp = (NaiveHashNode *)malloc(sizeof(NaiveHashNode));
    assert(NULL != *pp);
    (*pp)->key  = key;
    (*pp)->val  = val;
    (*pp)->next = NULL;

    ht->item_count++;
    return 0;
}

/*
 * @brief: 返回查找到的hash table 节点的指针。
 *         返回内部指针，考虑到外部可能会更改数据内容。
 *
 */
NaiveHashNode *
ht_lookup(const NaiveHashTable *ht, uint64_t key) {
    NaiveHashNode *p = ht->buckets[key % ht->buckets_size];
    while (NULL != p && p->key != key) {
        p = p->next;
    }
    return p;
}

/*
 * @brief:
 *
 */
const void *
ht_search(const NaiveHashTable *ht, uint64_t key) {
    NaiveHashNode *ptr = ht_lookup(ht, key);
    return NULL == ptr ? NULL : ptr->val;
}

double
cal_avg_load(const NaiveHashTable *ht) {
    return ht->item_count * 1.0 / ht->buckets_size;
}

/*------------------------- iterator operation ----------------------------*/
NaiveHashIter *
ht_iter_new(NaiveHashTable *ht) {
    assert(NULL != ht);
    NaiveHashIter *it = (NaiveHashIter *)malloc(sizeof(NaiveHashIter));
    it->ht            = ht;
    it->bucket_index  = ht->buckets_size; /* end */
    it->node_ptr      = NULL;
    return it;
}

NaiveHashIter *
ht_iter_del(NaiveHashIter *it) {
    assert(NULL != it);
    free(it);
    return NULL;
}

/* 
 * @brief: 会改变it参数的内容！
 */
NaiveHashIter *
ht_iter_begin(NaiveHashIter *it) {
    assert(NULL != it);
    unsigned long idx = 0LU;

    /* default : end */
    it->bucket_index = it->ht->buckets_size;
    it->node_ptr     = NULL;

    for (; idx < it->ht->buckets_size; idx++) {
        if (NULL != it->ht->buckets[idx]) {
            it->bucket_index = idx;
            it->node_ptr     = it->ht->buckets[idx];
            break;
        }
    }

    return it;
}

/* 
 * @brief: 会改变it参数的内容！
 *         hash table遍历出来的结果是无序的。 
 */
NaiveHashIter *
ht_iter_next(NaiveHashIter *it) {
    unsigned long idx = it->bucket_index;
    assert(NULL != it);
    assert(idx < it->ht->buckets_size);
    assert(NULL != it->node_ptr);

    /* 当前bucket下仍有node可以访问 */
    if (NULL != it->node_ptr->next) {
        it->node_ptr = it->node_ptr->next;
        return it;
    }

    /* 寻找下一个有内容的bucket */
    for (idx += 1; idx < it->ht->buckets_size; idx++) {
        if (NULL != it->ht->buckets[idx]) {
            it->node_ptr = it->ht->buckets[idx];
            break;
        }
    }
    /* note: 也可能是到了结尾。 */
    it->bucket_index = idx;
    return it;
}

int
ht_iter_isend(const NaiveHashIter *it) {
    assert(NULL != it);
    /* 只关注是否超过最后一个bucket */
    return (it->bucket_index == it->ht->buckets_size) ? 1 : 0;
}

NaiveHashNode *
ht_iter_entry(NaiveHashIter *it) {
    assert(NULL != it);
    return it->node_ptr;
}

/*----------------------------- iterator end ------------------------------*/

