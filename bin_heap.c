#include <stdlib.h>

#include "type.h"
#include "bin_heap.h"


PriorityQueue pq_create(int max_size, int (*cmp)(const void *, const void*)) {
    HeapElemType min = HEAP_MIN;

    PriorityQueue h = (PriorityQueue)malloc(sizeof(struct HeapStruct));
    h->size         = 0;
    h->capacity     = max_size + 1;
    h->elems        = (HeapElemType *)malloc(sizeof(HeapElemType) * h->capacity);
    h->elems[0]     = min; /* 哨兵 */
    h->compare      = cmp;
    return h;
}

void pq_destory(PriorityQueue h) {
    free(h->elems);
    free(h);
}

int pq_empty(PriorityQueue h) {
    return h->size < 1;
}

int pq_full(PriorityQueue h) {
    return h->size == h->capacity;
}

int pq_insert(PriorityQueue h, HeapElemType e) {
    int i = 0;
    if (h->size == h->capacity) {
        return -1;
    }

    for (i = ++h->size; (h->compare)(&h->elems[i/2], &e) > 0; i /= 2) {
        h->elems[i] = h->elems[i/2];
    }
    h->elems[i] = e;

    return 0;
}

HeapElemType pq_find_min(PriorityQueue h) {
    HeapElemType e = HEAP_MAX;
    if (h->size < 1) {
        return e;
    }
    return h->elems[1];
}

HeapElemType pq_delete_min(PriorityQueue h) {
    int i, child;
    HeapElemType e = HEAP_MAX;
    if (h->size < 1) {
        return e;
    }
    e = h->elems[1];

    HeapElemType last = h->elems[h->size--];

    for (i = 1; 2*i <= h->size; i = child) {
        child = 2 * i;
        if (child < h->size && (h->compare)(&h->elems[child], &h->elems[child+1]) > 0) {
            child++;
        }
        /* child < last */
        if ((h->compare)(&last, &h->elems[child]) > 0) {
            h->elems[i] = h->elems[child];
        } else {
            break;
        }
    }
    h->elems[i] = last;

    return e;
}
