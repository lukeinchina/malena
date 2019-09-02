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

static int term_inv_init(TermInvCell *inv) {
    memset(inv, sizeof(*inv), 0);

    inv->doc_size    = 1<<20; /* 按docic数目算 */
    inv->occ_byte    = 8<<20; /* 按byte算. */
    inv->doclist     = (docid_t *)malloc(sizeof(docid_t) * inv->doc_size);
    inv->occ_offsets = (uint32_t *)malloc(sizeof(uint32_t) * inv->doc_size);
    inv->occ_buff    = (uint8_t *)malloc(sizeof(uint8_t) * inv->occ_byte);

    return 0;
}

static void term_inv_reset(TermInvCell *inv, termid_t termid) {
    inv->termid  = termid;
    inv->doc_num = 0;
    inv->occ_len = 0;
}

static void term_inv_del(TermInvCell *inv) {
    free(inv->doclist);
    free(inv->occ_offsets);
    free(inv->occ_buff);
    memset(inv, sizeof(*inv), 0);
}

static int term_inv_expand_doc(TermInvCell *inv) {
    size_t bytes; 
    inv->doc_size   *= 2;
    bytes            = inv->doc_size * sizeof(docid_t);
    inv->doclist     = (docid_t *)realloc(inv->doclist, bytes);
    bytes            = inv->doc_size * sizeof(uint32_t);
    inv->occ_offsets = (uint32_t *)realloc(inv->occ_offsets, bytes);

    return 0;
}

int term_inv_expand_occ(TermInvCell *inv) {
    inv->occ_byte *= 2;
    inv->occ_buff   = (uint8_t *)realloc(inv->occ_buff, inv->occ_byte);
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

static int tv_load_term_inv(FILE *fp, TermInvCell *inv) {
    int      i;
    int      doc_num = 0;
    uint16_t occ_num = 0;
    size_t   offset  = sizeof(termid_t);
    size_t   length  = 0LU;


    /* docid list */
    fread(&doc_num, sizeof(doc_num), 1, fp);
    /* buffer 不足时重新分配内存空间*/
    while (inv->doc_num + doc_num > inv->doc_size) {
        term_inv_expand_doc(inv);
    }
    fread(inv->doclist + inv->doc_num, sizeof(docid_t), doc_num, fp);
    inv->doc_num += doc_num;
    offset += sizeof(doc_num) + sizeof(docid_t) * doc_num;

    /* occ list */
    for (i = 0; i < doc_num; i++) {
        /* save offsets */
        inv->occ_offsets[inv->doc_num++] = inv->occ_len;

        fread(&occ_num, sizeof(occ_num), 1, fp); 
        /* occ buffer 不足时重新分配内存空间*/
        while ( (inv->occ_len + sizeof(occ_num) + sizeof(occ_t) * occ_num)
                > inv->occ_byte) {
            term_inv_expand_occ(inv);
        }
        memcpy(inv->occ_buff + inv->occ_len, &occ_num, sizeof(occ_num));
        inv->occ_len += sizeof(occ_num);
        fread(inv->occ_buff + inv->occ_len, sizeof(occ_t), occ_num, fp);
        inv->occ_len += sizeof(occ_t) * occ_num;

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
static int term_inv_dump(TermInvCell *inv, FILE *fp) {
    uint32_t total_byte = 0;
    total_byte += sizeof(termid_t);
    total_byte += sizeof(uint32_t);
    total_byte += sizeof(docid_t) * inv->doc_num;
    total_byte += sizeof(uint32_t) * inv->doc_num;
    total_byte += inv->occ_len;

    fwrite(&total_byte, sizeof(total_byte), 1, fp);

    fwrite(&inv->termid, sizeof(termid_t), 1, fp);
    fwrite(&inv->doc_num, sizeof(uint32_t), 1, fp);
    fwrite(inv->doclist, sizeof(docid_t), inv->doc_num, fp);
    fwrite(inv->occ_offsets, sizeof(uint32_t), inv->doc_num, fp);
    fwrite(inv->occ_buff, 1, inv->occ_len, fp);

    return 0;
}

/*
 * @return: total termid num.
 */
int multiway_merge_invert(PriorityQueue queue, const char *invert_dir) {
    FILE *fp;
    FILE *inv_fp;
    char path[PATH_MAX];
    int  term_total   = 0;

    HeapElemType prev = HEAP_MIN;
    HeapElemType curr = HEAP_MIN;
    HeapElemType e;

    TermInvCell inv;
    term_inv_init(&inv);

    snprintf(path, sizeof(path), "%s/%s", invert_dir, INVERT_NAME);
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
            term_inv_reset(&inv, curr.key1);
        } else if (IS_NE(curr, prev)) {
            term_inv_dump(&inv, inv_fp);
            term_inv_reset(&inv, curr.key1);
            term_total += 1;
        }

        fp = curr.load;
        tv_load_term_inv(fp, &inv);

        /* 下一个元素入堆 */
        e = curr;
        e.key1 = tv_load_termid(fp);
        pq_insert(queue, e);
    }

    if (inv.doc_num > 0) {
        term_inv_dump(&inv, inv_fp);
        term_total += 1;
    }

    fclose(inv_fp);
    term_inv_del(&inv);
    return term_total;
}

/*
 * @breif: 多个临时倒排文件，合并成一个倒排文件。
 * @return: total termid num
 */
int merge_temp_inverts(char **temp_inv_paths, size_t size,
        const char *invert_dir) {
    FILE  *fp         = NULL;
    size_t i          = 0;
    int    term_total = -1;
    PriorityQueue queue;
    assert(size > 0);

    queue = pq_create(size, greater);

    /* 初始化多路归并的优先队列 */
    HeapElemType *lines = (HeapElemType *)malloc(sizeof(HeapElemType) * size);
    for (i = 0; i < size; i++) {
        fp = fopen(temp_inv_paths[i], "r");
        assert(NULL != fp);
        if (tv_check_head(fp) != 0) {
            LOG(LOG_ERROR, "check temp invert [%s] failed", temp_inv_paths[i]);
            goto merge_end;
        }
        lines[i].load = fp;
        lines[i].key1 = tv_load_termid(fp);
        lines[i].key2 = i;

        if (pq_insert(queue, lines[i]) != 0) {
            LOG(LOG_ERROR, "insert priority queue failed");
            goto merge_end;
        }
    }

    /* 多路归并，合并出最终的term对应的倒排 */
    term_total = multiway_merge_invert(queue, invert_dir);
    if (term_total < 0) {
        LOG(LOG_ERROR, "mult-way merge failed");
    }

merge_end:

    for (i = 0; i < size; i++) {
        fp = (FILE *)(lines[i].load);
        if (NULL != fp) {
            fclose(fp);
        }
    }
    free(lines);
    pq_destory(queue);

    return term_total;
}
