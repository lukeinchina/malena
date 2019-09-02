#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include "log.h"
#include "type.h"
#include "define.h"
#include "naive_hash_table.h"
#include "merge.h"
#include "search.h"

static TermInvCell *all_terms = NULL;
static char *invert_buff      = NULL;

off_t get_file_size(const char *path)
{
	off_t size = -1;	
	struct stat statbuff;
	if(stat(path, &statbuff) == 0){
		size = statbuff.st_size;
	}
	return size;
}

static int get_invert_meta(const char *path, Meta *meta)
{
    int  value;
    char line[512];
    char errstr[512];
    FILE *fp = fopen(path, "r");
    if (NULL == fp) {
        strerror_r(errno, errstr, sizeof(errstr));
        LOG(LOG_ERROR, "open [%s] read failed:%s", path, errstr);
        return -1;
    }
    while (fgets(line, sizeof(line), fp) != NULL) {
        if(sscanf(line, "doc num:%d\n", &value) == 1) {
            meta->doc_num = value; 
        }
        if(sscanf(line, "term num:%d\n", &value) == 1) {
            meta->term_num = value; 
        }
    }
    fclose(fp);
    return 0;
}

static char* load_term_invert(const char *inv_path)
{
    char *buff = NULL;
    long size = -1;
    char  errstr[512];
    FILE *fp = fopen(inv_path, "r");
    if (NULL == fp) {
        strerror_r(errno, errstr, sizeof(errstr));
        LOG(LOG_ERROR, "open [%s] read failed:%s", inv_path, errstr);
        return NULL;
    }
    do {
        fseek(fp, 0L, SEEK_END);
        size = ftell(fp);
        fseek(fp, 0L, SEEK_SET);
        buff = (char *)malloc(size);
        if (size < 1) {
            LOG(LOG_ERROR, " [%s] file size:%ld", inv_path, size);
            break;
        }
        if (fread(buff, 1, size, fp) < (size_t)size) {
            LOG(LOG_ERROR, " read file [%s] failed", inv_path);
            break;
        }
    } while (0);
    fclose(fp);
    return buff;
}

NaiveHashTable *load_invert_index(const char *invert_dir)
{
    uint32_t total_len;
    int i;
    char *p, *q;
    Meta meta = {0, 0};
    NaiveHashTable *ht = NULL;
    char path[PATH_MAX];
    TermInvCell *cell = NULL;


    snprintf(path, sizeof(path), "%s/%s", invert_dir, META_NAME);
    if (get_invert_meta(path, &meta) != 0) {
        LOG(LOG_ERROR, "get invert meta failed\n");
        return NULL;
    }
    LOG(LOG_INFO, "doc num:%d, term num:%d", meta.doc_num, meta.term_num);

    ht = ht_create(meta.term_num / 2);
    assert(NULL != ht);

    snprintf(path, sizeof(path), "%s/%s", invert_dir, INVERT_NAME);
    invert_buff = load_term_invert(path);
    all_terms   = (TermInvCell *)malloc(sizeof(TermInvCell) * meta.term_num);

    p = invert_buff;
    for (i = 0; i < meta.term_num; i++) {
        q = p;
        cell = all_terms + i;

        /* total length */
        total_len = *(uint32_t*)p;
        p += sizeof(total_len);

        /* termid */
        cell->termid = *(termid_t*)p;
        p += sizeof(cell->termid);

        /* doc num */
        cell->doc_num = *(uint32_t*)p;
        p += sizeof(cell->doc_num);
        cell->doc_size = cell->doc_num; /* not used */

        cell->doclist = (docid_t *)p;
        p += cell->doc_num * sizeof(docid_t);
        cell->occ_offsets = (uint32_t*)p;
        p += cell->doc_num * sizeof(uint32_t);

        /* occ begin */
        cell->occ_buff = (uint8_t *)p;
        cell->occ_len  = total_len + sizeof(total_len) - (p - q);
        cell->occ_byte = cell->occ_len; /* not used */

        ht_insert(ht, cell->termid, cell);

        /* next termid */
        p = q + total_len + sizeof(total_len);
    }
    assert((p - invert_buff) == get_file_size(path));

    return ht;
}

void unload_invert_index(NaiveHashTable *ht)
{
    free(all_terms);
    free(invert_buff);
    ht_clear_items(ht);
    ht_destory(ht);
    all_terms = NULL;
    invert_buff = NULL;
    return;
}
