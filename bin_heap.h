#ifndef MALENA_BIN_HEAP_H
#define MALENA_BIN_HEAP_H

#include <limits.h>

#define HEAP_MIN {NULL, 0, 0}
#define HEAP_MAX {NULL, ULLONG_MAX, ULLONG_MAX}

typedef struct {
    void *load;
    uint64_t key1;
    uint64_t key2;
}HeapElemType;

struct HeapStruct;
typedef struct HeapStruct *PriorityQueue;

PriorityQueue pq_create(int max_size, int (*cmp)(const void *, const void*));
void pq_destory(PriorityQueue h);
void pq_make_empty(PriorityQueue h);
int  pq_insert(PriorityQueue h, HeapElemType e);
HeapElemType pq_delete_min(PriorityQueue h);
HeapElemType pq_find_min(PriorityQueue h);
int pq_empty(PriorityQueue h);
int pq_full(PriorityQueue h);

struct HeapStruct {
    int capacity;
    int size;
    HeapElemType *elems;
    int (*compare)(const void *, const void*);
};

#endif
