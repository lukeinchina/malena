#ifndef MALENA_INDEX_H
#define MALENA_INDEX_H

#include "naive_hash_table.h"

typedef struct {
	char *title;
	char *content;
}Doc;

int create_static_index(const char *seg_path, const char *invert_dir);

#endif
