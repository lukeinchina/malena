#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <limits.h>

#include "log.h"
#include "type.h"
#include "merge.h"
#include "bin_heap.h"
#include "define.h"

#define IS_MAX(e) ((e).key1 == ULLONG_MAX)
#define IS_EQ(e1, e2) ((e1).key1 == (e2).key1)
#define IS_NE(e1, e2) ((e1).key1 != (e2).key1)
#define IS_MIN(e) ((e).key1 == 0LLU)

/*
 * 合并多个临时倒排后，一个termid所对应的docid和位置信息。
 * 统一存储，然后写入最终的倒排文件中。
 */
typedef struct {
    termid_t termid;

    uint32_t doc_size;
    uint32_t doc_num;
    docid_t  *doclist;
    /* 记录每个docid内位置记录的开始地址/偏移量 */
    uint32_t *occ_offsets;

    /* 位置记录,格式为{{occ_num}{occ}+ }+ */
    uint32_t occ_byte;
    uint32_t occ_len;
    uint8_t  *occ_buff;   

    uint32_t  offset;

}TermInvRecord;

static int term_inv_init(TermInvRecord *rec) {
    memset(rec, sizeof(*rec), 0);

    rec->doc_size    = 1<<20; /* 按docic数目算 */
    rec->occ_byte    = 8<<20; /* 按byte算. */
    rec->doclist     = (docid_t *)malloc(sizeof(docid_t) * rec->doc_size);
    rec->occ_offsets = (uint32_t *)malloc(sizeof(uint32_t) * rec->doc_size);
    rec->occ_buff    = (uint8_t *)malloc(sizeof(uint8_t) * rec->occ_byte);

    return 0;
}

static void term_inv_reset(TermInvRecord *rec, termid_t termid) {
    rec->termid  = termid;
    rec->doc_num = 0;
    rec->occ_len = 0;
    rec->offset  = 0;
}

static void term_inv_del(TermInvRecord *rec) {
    free(rec->doclist);
    free(rec->occ_offsets);
    free(rec->occ_buff);
    memset(rec, sizeof(*rec), 0);
}

static int term_inv_expand_doc(TermInvRecord *rec) {
    size_t bytes; 
    rec->doc_size   *= 2;
    bytes            = rec->doc_size * sizeof(docid_t);
    rec->doclist     = (docid_t *)realloc(rec->doclist, bytes);
    bytes            = rec->doc_size * sizeof(uint32_t);
    rec->occ_offsets = (uint32_t *)realloc(rec->occ_offsets, bytes);

    return 0;
}

int term_inv_expand_occ(TermInvRecord *rec) {
    rec->occ_byte *= 2;
    rec->occ_buff   = (uint8_t *)realloc(rec->occ_buff, rec->occ_byte);
    return 0;
}

/*
 * @brief:比较函数，用做指针传递给小顶堆做比较。
 *
 */
int greater(const void *left, const void *right) {
    const HeapElemType *l = (const HeapElemType *)left;
    const HeapElemType *r = (const HeapElemType *)right;
    if (l->key1 > r->key1
            || (l->key1 == r->key1 && l->key2 > r->key2)) {
        return 1;
    } else if (l->key1 == r->key1 && l->key2 == r->key2) {
        return 0;
    } else {
        return -1;
    }
}

/*
 * tv : temporary invert.
 */
static int tv_check_head(FILE *fp) {
    struct TempInvertHead head;
    memset(&head, sizeof(head), 0);
    fread(&head, sizeof(head), 1, fp);
    if (head.termid_num < 1) {
        return -1;
    }
    return 0;
}

/*
 * @brief: 读取不到termid（即文件结束），返回一个最大值.
 */
static termid_t tv_load_termid(FILE *fp) {
    termid_t id;
    if (fread(&id, sizeof(id), 1, fp) < 1) {
        id = ULLONG_MAX;
    }
    return id;
}

static int get_invert_path(char *path, size_t size, const char *invert_dir) {
    return snprintf(path, size, "%s/%s", invert_dir, INVERT_NAME);
}

static int tv_load_term_inv(FILE *fp, TermInvRecord *rec) {
    int      i;
    int      doc_num = 0;
    uint16_t occ_num = 0;
    size_t   offset  = sizeof(termid_t);
    size_t   length  = 0LU;


    /* docid list */
    fread(&doc_num, sizeof(doc_num), 1, fp);
    /* buffer 不足时重新分配内存空间*/
    while (rec->doc_num + doc_num > rec->doc_size) {
        term_inv_expand_doc(rec);
    }
    fread(rec->doclist + rec->doc_num, sizeof(docid_t), doc_num, fp);
    rec->doc_num += doc_num;
    offset += sizeof(doc_num) + sizeof(docid_t) * doc_num;

    /* occ list */
    for (i = 0; i < doc_num; i++) {
        /* save offsets */
        rec->occ_offsets[rec->offset++] = rec->occ_len;

        fread(&occ_num, sizeof(occ_num), 1, fp); 
        /* occ buffer 不足时重新分配内存空间*/
        while ( (rec->occ_len + sizeof(occ_num) + sizeof(occ_t) * occ_num)
                > rec->occ_byte) {
            term_inv_expand_occ(rec);
        }
        memcpy(rec->occ_buff + rec->occ_len, &occ_num, sizeof(occ_num));
        rec->occ_len += sizeof(occ_num);
        fread(rec->occ_buff + rec->occ_len, sizeof(occ_t), occ_num, fp);
        rec->occ_len += sizeof(occ_t) * occ_num;

        offset += sizeof(occ_num) + sizeof(occ_t) * occ_num;
    }

    /* check */
    fread(&length, sizeof(length), 1, fp);
    if (length != offset) {
        printf("%lu != %lu\n", length, offset);
    }
    assert(length == offset);
    return 0;
}

/*
 *@brief: 倒排文件中的字段格式如下:
 *   total_len  termid, doc_num, {docid}+, {offset}+, {{occ_num}{occ}+ }+
 *   | 4 byte|  |8 byte| | 4byte| | 4byte|+ |4 byte|+  
 *
 */
static int term_inv_dump(TermInvRecord *rec, FILE *fp) {
    uint32_t total_byte = 0;
    total_byte += sizeof(termid_t);
    total_byte += sizeof(uint32_t);
    total_byte += sizeof(docid_t) * rec->doc_num;
    total_byte += sizeof(uint32_t) * rec->doc_num;
    total_byte += rec->occ_len;

    fwrite(&total_byte, sizeof(total_byte), 1, fp);

    fwrite(&rec->termid, sizeof(termid_t), 1, fp);
    fwrite(&rec->doc_num, sizeof(uint32_t), 1, fp);
    fwrite(rec->doclist, sizeof(docid_t), rec->doc_num, fp);
    fwrite(rec->occ_offsets, sizeof(uint32_t), rec->doc_num, fp);
    fwrite(rec->occ_buff, 1, rec->occ_len, fp);

    assert(rec->doc_num == rec->offset);
    return 0;
}

int multiway_merge_invert(PriorityQueue queue, const char *invert_dir) {
    FILE *fp;
    FILE *inv_fp;
    char path[PATH_MAX];

    HeapElemType prev = HEAP_MIN;
    HeapElemType curr = HEAP_MIN;
    HeapElemType e;

    TermInvRecord rec;
    term_inv_init(&rec);

    get_invert_path(path, sizeof(path), invert_dir);
    inv_fp = fopen(path, "w");
    if (NULL == inv_fp) {
        LOG(LOG_FATAL, "open [%s] to write failed", path);
        return -1;
    }

    for (; ; prev = curr) {
        /* MAX 的id表示取到的数据是空，全部结束了。*/
        curr = pq_delete_min(queue);
        if (IS_MAX(curr)) {
            break;
        }

        if (IS_MIN(prev)) {
            term_inv_reset(&rec, curr.key1);
        } else if (IS_NE(curr, prev)) {
            term_inv_dump(&rec, inv_fp);
            term_inv_reset(&rec, curr.key1);
        }

        fp = curr.load;
        tv_load_term_inv(fp, &rec);

        /* 下一个元素入堆 */
        e = curr;
        e.key1 = tv_load_termid(fp);
        pq_insert(queue, e);
    }

    if (rec.doc_num > 0) {
        term_inv_dump(&rec, inv_fp);
    }

    fclose(inv_fp);
    term_inv_del(&rec);
    return 0;
}

int merge_temp_inverts(char **temp_inv_paths, size_t size,
        const char *invert_dir) {
    FILE *fp = NULL;
    PriorityQueue queue;
    size_t i;
    int  err = 0;
    assert(size > 0);

    queue = pq_create(size, greater);

    /* 初始化多路归并的优先队列 */
    HeapElemType *lines = (HeapElemType *)malloc(sizeof(HeapElemType) * size);
    for (i = 0; i < size; i++) {
        fp = fopen(temp_inv_paths[i], "r");
        assert(NULL != fp);
        if (tv_check_head(fp) != 0) {
            LOG(LOG_ERROR, "check temp invert [%s] failed", temp_inv_paths[i]);
            err = -1;
            break;
        }
        lines[i].load = fp;
        lines[i].key1 = tv_load_termid(fp);
        lines[i].key2 = i;

        if (pq_insert(queue, lines[i]) != 0) {
            LOG(LOG_ERROR, "insert priority queue failed");
            err = -1;
            break;
        }
    }

    /* 多路归并，合并出最终的term对应的倒排 */
    if (multiway_merge_invert(queue, invert_dir) != 0) {
        LOG(LOG_ERROR, "mult-way merge failed");
        err = -1;
    }

    for (i = 0; i < size; i++) {
        fp = (FILE *)(lines[i].load);
        if (NULL != fp) {
            fclose(fp);
        }
    }
    free(lines);
    pq_destory(queue);

    return err;
}
