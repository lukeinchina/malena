#ifndef MALENA_SEARCH_H
#define MALENA_SEARCH_H

#include "naive_hash_table.h"

NaiveHashTable *load_invert_index(const char *inv_path);
void unload_invert_index(NaiveHashTable *ht);

#endif
