#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
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

int query_to_termids(const char *query, termid_t *termids, int size)
{
    termid_t *ptr = termids;
    char word[256];
    const uint8_t *start = (const uint8_t *)query;
    const uint8_t *end   = start; 
    while (0 != *start) {
        for (; 0 != *start && isspace(*start); start++) {}
        end = start;
        for (; 0 != *end && !isspace(*end); end++) {}
        if (start == end) {
            break;
        }

        *ptr  = termid(start, end);
        strncpy(word, (const char *)start, end-start);
        word[end-start] = 0;
        // printf("[%s:%llu %d]  ", word, ptr->id, ptr->occ);
        ptr      += 1;

        if (ptr - termids >= size) {
            LOG(LOG_WARN, "term count[%ld] is overflow. size=[%d]", ptr - termids, size);
            break;
        }

        start = end;
    }
    return ptr - termids;
}

int fetch_term_index(NaiveHashTable *ht, const termid_t *termids, int size,
        const TermInvCell **term_invs)
{
    int i;
    int count = 0;
    TermInvCell *ptr;
    for (i = 0; i < size; i++) {
        ptr = (TermInvCell *)ht_search(ht, termids[i]);
        if (NULL == ptr) {
            continue;
        }
        term_invs[count++] = ptr;
    }
    return count;
}

int cmp_by_doc_num(const void *left, const void *right)
{
    int a = (*(const TermInvCell **)left)->doc_num;
    int b = (*(const TermInvCell **)right)->doc_num;
    return  (a - b);
}

int lower_bound(const docid_t *array, int left, int right, docid_t key)
{
    int mid;
    while (left <= right) {
        mid = (left + right) / 2;
        if (key < array[mid]) {
            right = mid - 1;
        } else if(key > array[mid]) {
            left = mid + 1;
        } else {
            return mid;
        }
    }
    return left;
}

int common_by_traverse(const TermInvCell **term_invs, int size, 
        docid_t *dst, int dst_size)
{
    (void)term_invs;
    (void)size;
    (void)dst;
    (void)dst_size;
    int count = 0;
    return count;
}

int common_by_bsearch(const TermInvCell **term_invs, int size, 
        docid_t *dst, int dst_size)
{
    int i, fin, count, off;
    docid_t target;
    const TermInvCell *terms[QUERY_TERM_MAX];
    int offsets[QUERY_TERM_MAX] = {0};

    for (i = 0; i < size; i++) {
        terms[i] = term_invs[i];
    }
    qsort(terms, size, sizeof(TermInvCell *), cmp_by_doc_num);

    count = 0;
    fin   = 0;
    target = terms[0]->doclist[0];
    while (count < dst_size) {
        for (i = 0; i < size; ) {
            off = lower_bound(terms[i]->doclist, offsets[i], terms[i]->doc_num, target);
            offsets[i] = off;
            if (off >= (int)terms[i]->doc_num) {
                fin = 1;
                break;
            } else if (target != terms[i]->doclist[off]) {
                /* 没有找到相同元素 */
                target = terms[i]->doclist[off];
                i = 0; /* rewind */
            } else {
                i += 1;
            }
        }
        if (fin) {
            break;
        }
        /* debug info */
        assert(i == size);
        for (i = 0; i < size; i++) {
            assert(target == terms[i]->doclist[offsets[i]]);
        }

        dst[count++] = target;
        target += 1;
    }
    return count;
}

int common_docs(const TermInvCell **term_invs, int size, 
        docid_t *dst, int dst_size)
{
    int i;
    uint32_t min, max;

    if (size == 1) {
        dst_size = (int)term_invs[0]->doc_num > dst_size ? dst_size : term_invs[0]->doc_num;
        memcpy(dst, term_invs[0]->doclist, dst_size * sizeof(docid_t));
        return dst_size;
    }
    min = max = term_invs[0]->doc_num;
    for (i = 1; i < size; i++) {
        if (term_invs[i]->doc_num > max) {
            max = term_invs[i]->doc_num;
        }
        if (term_invs[i]->doc_num < min) {
            min = term_invs[i]->doc_num;
        }
    }
    return common_by_bsearch(term_invs, size, dst, dst_size);
    if (min < max / 10) {
        return common_by_bsearch(term_invs, size, dst, dst_size);
    } else {
        return common_by_traverse(term_invs, size, dst, dst_size);
    }
}
