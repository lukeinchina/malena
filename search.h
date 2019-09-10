#ifndef MALENA_SEARCH_H
#define MALENA_SEARCH_H

#include "naive_hash_table.h"
#include "util.h"

typedef struct {
    termid_t termid;
    char *word;      /* for human reading */
}QueryTerm;


NaiveHashTable *load_invert_index(const char *inv_path);
void unload_invert_index(NaiveHashTable *ht);

int query_to_termids(const char *query, termid_t *termids, int size);

int common_docs(const TermInvCell **query_terms, int size, docid_t *dst,
        int dst_size);
int fetch_term_index(NaiveHashTable *ht, const termid_t *termids, int size,
        const TermInvCell **dst);

#endif
