#ifndef MALENA_INDEX_H
#define MALENA_INDEX_H

typedef struct {
	char *title;
	char *content;
}Doc;

int index_one_doc(const Doc *doc);

#endif
