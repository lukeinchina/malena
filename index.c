#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include "define.h"
#include "type.h"
#include "util.h"
#include "log.h"
#include "index.h"
#include "naive_hash_table.h"


typedef struct {
    termid_t id;
    occ_t    occ;
}Term;

#define DEF_OCC_SIZE 2
/* */
#pragma pack(2)
typedef struct {
    uint16_t size;
    uint16_t count;
    occ_t occs[];
}TermOcc;
#pragma pack()

/* */
#pragma pack(2)
typedef struct {
    docid_t docid;
    uint16_t count;
    occ_t    occs[];
}DocOcc;
#pragma pack()

/*
 * @brief:  hash表中存放的索引数据示意图。
 * 
 * ________
 * |  0   |
 * |______|   ___________________   ___________________  
 * |  1   |-->|      termid     |-->|      termid     |--> ...
 * |______|   |-----------------|   |-----------------|    
 * |  2   |   |InvertListHead * |   |InvertListHead * |    
 * |______|   |-----------------|   |-----------------|    
 * |  .   |          | 
 * |  .   |       LinkedList
 * |  .   |       -------    struct DocOcc
 * |  .   |       |_____|--->{docid, count, occ*}
 * |  .   |          |   
 * |  .   |       -------
 * |  .   |       |_____|--->{docid, count, occ*}
 * |______|          |
 * |  i   |       -------
 * |______|       |_____|---> ...
 * |  N   |        
 * |______|
 */
struct LinkedNode;
typedef struct LinkedNode LinkedList;
struct LinkedNode {
    void *val;
    LinkedList *next;
};

typedef struct {
    int size; 
    int occ_num;
    LinkedList *head;
    LinkedList *tail;
}InvertListHead;

int termid_cmp_inc(const void *left, const void *right) {
    return ((termid_t)left < (termid_t)right) ? -1 : 1;
}

static
TermOcc *create_term_occs(occ_t occ) {
    size_t mem_size = sizeof(TermOcc) + sizeof(occ_t) * DEF_OCC_SIZE;
    TermOcc *head   = (TermOcc *)malloc(mem_size);
    head->size      = DEF_OCC_SIZE;
    head->count     = 1;
    head->occs[0]   = occ;
    return head;
}

static
void free_term_occs(TermOcc *head) {
    free(head);
    return;
}

static
TermOcc *append_term_occs(TermOcc *head, occ_t occ) {
    size_t mem_size = 0;
    if (head->count == head->size) {
        /* 扩大一倍 occ的空间 */
        mem_size    = sizeof(TermOcc) + sizeof(occ_t) * head->size * 2;
        head        = (TermOcc *)realloc(head, mem_size);
        head->size *= 2;
    }
    head->occs[head->count++] = occ;

    return head;
}

static
LinkedList *create_list_node(docid_t id, const TermOcc *occ_head) {

    LinkedList *list = (LinkedList *)malloc(sizeof(LinkedList));

    size_t mem_size = sizeof(DocOcc) + occ_head->count * sizeof(occ_t);
    DocOcc *head    = (DocOcc*)malloc(mem_size);
    head->docid     = id;
    head->count     = occ_head->count;
    memcpy(head->occs, occ_head->occs, sizeof(occ_t) * head->count);

    list->val  = head;
    list->next = NULL;
    return list;
}

static
void free_list_node(LinkedList *list) {
    free(list->val);
    free(list);
    return;
}

static int
text_to_term(const char *text, int offset, Term *terms, int size) {
    Term *ptr = terms;
    char word[256];
    const uint8_t *start = (const uint8_t *)text;
    const uint8_t *end   = start; 
    while (0 != *start) {
        for (; 0 != *start && isspace(*start); start++) {}
        end = start;
        for (; 0 != *end && !isspace(*end); end++) {}
        if (start == end) {
            break;
        }

        ptr->id  = termid(start, end);
        ptr->occ = offset;
        offset   += utf8_count(start, end);
        strncpy(word, (const char *)start, end-start);
        word[end-start] = 0;
        // printf("[%s:%llu %d]  ", word, ptr->id, ptr->occ);
        ptr      += 1;

        if (ptr - terms >= size) {
            LOG(LOG_WARN, "term count[%ld] is overflow. size=[%d]", ptr - terms, size);
            break;
        }

        start = end;
    }
    return (int)(ptr - terms);
}

/*
 * @brief:
 * @param doc [in]:
 * @return:
 */
static int
index_one_doc(NaiveHashTable *ht, const Doc *doc) {
    int i, n;

    Term terms[MAX_TERM_NUM];

    NaiveHashNode  *node     = NULL;
    TermOcc        *occ_head = NULL;

    n = text_to_term(doc->title, 0, terms, MAX_TERM_NUM);
    /* check n */
    n += text_to_term(doc->content, MAX_TITLE_LEN, terms+n, MAX_TERM_NUM - n);
    if (n < 1) {
        return 0;
    }

    /* 所有的term & occ 加到hash表中，形成索引结构。*/
    for (i = 0; i < n; i++) {
        node = ht_lookup(ht, terms[i].id);
        if (NULL == node) {
            occ_head = create_term_occs(terms[i].occ);
            ht_insert(ht, terms[i].id, occ_head);
        } else {
            node->val = append_term_occs(node->val, terms[i].occ);
        }
    }
    NaiveHashIter *it = ht_iter_new(ht);
    for (it = ht_iter_begin(it); ht_iter_isend(it) == 0; it = ht_iter_next(it)){
        node = ht_iter_entry(it);
        occ_head = (TermOcc *)node->val;

        /* printf("[%llu] ", node->key);
        for (i = 0; i < occ_head->count; i++) {
            printf("%d\t", occ_head->occs[i]);
        }
        printf("\n");
        */
    }
    it = ht_iter_del(it);

    return n;
}

static void 
free_term_occ_in_ht(NaiveHashTable *ht) {
    NaiveHashNode *node = NULL;
    NaiveHashIter *it   = ht_iter_new(ht);
    for (it = ht_iter_begin(it); ht_iter_isend(it) == 0; it = ht_iter_next(it)){
        node = ht_iter_entry(it);
        free_term_occs(node->val);
        node->val = NULL;
    }
    it = ht_iter_del(it);
    return;
}

static void
free_doc_occ_in_ht(NaiveHashTable *ht) {
    InvertListHead *list  = NULL;
    NaiveHashNode  *node  = NULL;
    LinkedList     *p, *q;

    NaiveHashIter *it = ht_iter_new(ht);
    for (it = ht_iter_begin(it); ht_iter_isend(it) == 0; it = ht_iter_next(it)){
        node = ht_iter_entry(it);
        list = (InvertListHead *)(node->val);
        p    = list->head;

        /* 释放链表 */
        while (NULL != p) {
            q  = p->next;
            free_list_node(p);
            p = q;
        }
        
        /* 释放链表头 */
        free(node->val);
        node->val = NULL;
    }
    it = ht_iter_del(it);
    return;
}


static uint64_t *
get_ht_keys(const NaiveHashTable *ht, size_t *size) {

    size_t idx          = 0;
    size_t array_size   = (size_t)ht_item_count(ht);
    uint64_t *key_array = (termid_t *)malloc(sizeof(termid_t) * array_size);
    NaiveHashNode  *node = NULL;

    NaiveHashIter *it = ht_iter_new((NaiveHashTable *)ht);
    for (it = ht_iter_begin(it); !ht_iter_isend(it); it = ht_iter_next(it)){
        node = ht_iter_entry(it);
        key_array[idx++] = node->key;
    }
    it = ht_iter_del(it);

    assert(idx == array_size);
    *size = array_size;
    return key_array;
}

/*
 * @brief:  
 * 每个termid对应的临时倒排文件的二进制数据格式如下:(64bit system)
 * {8 byte}{4 byte} {4byte,  2byte }+, { {2byte}+, {2byte}+,...}  {8 byte}
 * termid, doc_num, {docid, occ_num}+, { { occ }+, { occ }+,...}, total_length
 *
 */
static int
dump_temp_invert(NaiveHashTable *ht, const char *invert_path) {
    InvertListHead *list  = NULL;
    NaiveHashNode  *node  = NULL;
    LinkedList     *ptr   = NULL;
    DocOcc   *occ_head    = NULL;

    size_t max_size  = 128 * 1024 * 1024; /* 128M byte */
    size_t offset    = 0;
    size_t total_len = 0;

    size_t occ_byte = 0;
    char  *occ_buff = (char *)malloc(max_size);
    char  *curr     = occ_buff;

    /* 取到所有的termid，并递增排序 */
    size_t idx, array_size;
    termid_t *termid_array = (termid_t *)get_ht_keys(ht, &array_size);
    qsort(termid_array, array_size, sizeof(termid_t), termid_cmp_inc);

    FILE *fp = fopen(invert_path, "w");
    assert(NULL != fp);

    for (idx = 0; idx < array_size; idx++) {
        node = ht_lookup(ht, termid_array[idx]);
        list = (InvertListHead *)(node->val);

        offset     = 0;
        total_len  = 0;
        /* 1. termid和doc数目 */
        total_len += fwrite(&(node->key), sizeof(node->key), 1, fp);
        total_len += fwrite(&(list->size), sizeof(list->size), 1, fp);

        for (ptr = list->head; NULL != ptr; ptr = ptr->next) {
            /* 2. docid + occ_num */
            occ_head = (DocOcc *)(ptr->val);
            total_len += fwrite(occ_head, sizeof(*occ_head), 1, fp);

            /* 先把位置信息拷贝到缓冲区中，后面一次性写入 */
            curr     = occ_buff + offset;
            occ_byte = occ_head->count * sizeof(occ_t);
            if ((offset + occ_byte) > max_size) {
                max_size *= 2;
                occ_buff  = realloc(occ_buff, max_size);
                curr      = occ_buff + offset;
            }
            memcpy(curr, occ_head->occs, occ_byte);
            offset += occ_byte;
        }

        fwrite(occ_buff, offset, 1, fp);
        /* 3. 写入位置信息 */
        total_len += offset;
        /* 4. 最后写入总长度，相当于哨兵，起到一个校验作用 */
        fwrite(&total_len, sizeof(total_len), 1, fp);
    }

    fclose(fp);

    free(termid_array);
    free(occ_buff);
    return 0;
}

static int
append_into_index(NaiveHashTable *dst, NaiveHashTable *src, docid_t docid) {

    InvertListHead *list     = NULL;
    LinkedList     *load     = NULL;
    NaiveHashIter  *it       = NULL;
    NaiveHashNode  *src_node = NULL;
    NaiveHashNode  *dst_node = NULL;

    DocOcc *doc_occ = NULL;

    it = ht_iter_new(src);
    for (it = ht_iter_begin(it); ht_iter_isend(it) == 0; it = ht_iter_next(it)){
        /* 生成待插入的节点 */
        src_node = ht_iter_entry(it);
        load = create_list_node(docid, (TermOcc *)(src_node->val));

        /* 取到termid对应的倒排链表 */
        dst_node = ht_lookup(dst, src_node->key);
        if (NULL == dst_node) {
            list = (InvertListHead *)malloc(sizeof(InvertListHead));
            list->size = list->occ_num = 0;
            list->tail = list->head    = NULL;
            assert(ht_insert(dst, src_node->key, list) == 0);
        } else {
            list = dst_node->val;
        }

        /* 新的docid相关的索引信息，插入到链表尾部. */
        if (NULL == list->tail) {
            list->tail = list->head = load;
        } else {
            list->tail->next = load;
            list->tail = load;
        }
        /* ugly !*/
        doc_occ = (DocOcc *)(load->val);
        list->size    += 1;
        list->occ_num += doc_occ->count;
    }
    it = ht_iter_del(it);
    return 0;
}

int
create_static_index(const char *seg_path, const char *invert_dir) {
    char path[512];
    char buff[256];
    char title[4096];
    char content[65536];
    char err_msg[256];
    Doc  one;
    int  succ_count = 0;
    int  count = 0;
    int  no    = 0;
    FILE *fd   = NULL;

    NaiveHashTable *multi_doc_ht = ht_create(100000);
    NaiveHashTable *one_doc_ht   = ht_create(4000);

    fd = fopen(seg_path, "r");
    if (NULL == fd) {
        strerror_r(errno, err_msg, sizeof(err_msg));
        LOG(LOG_FATAL, "open [%s] to read failed:%s\n", seg_path, err_msg);
        return -1;
    }

    while (1) {
        /*  文件格式是一行标题，一行内容，一行空 */
        if ((NULL == fgets(title, sizeof(title), fd)) 
                || (NULL == fgets(content, sizeof(content), fd))
                || (NULL == fgets(buff, sizeof(buff), fd))) {
            break;
        }
        one.title   = title;
        one.content = content;

        count++;
        index_one_doc(one_doc_ht, &one);
        append_into_index(multi_doc_ht, one_doc_ht, (docid_t)count);

        succ_count++;
        /* 释放当前文档的索引空间，为下次做准备  */
        free_term_occ_in_ht(one_doc_ht);
        ht_clear_items(one_doc_ht); 

        if (count % TEMP_INVERT_DOC_SIZE != 0) {
            continue;
        }

        /* 索引文档数目到阈值了，生成一次临时倒排文件写入磁盘 */
        snprintf(path, sizeof(path), "%s/%s.%03d", invert_dir, 
                       TEMP_INVERT_NAME, no);
        dump_temp_invert(multi_doc_ht, path);
        no++;
        /* 释放本批次的索引空间，为下次做准备  */
        free_doc_occ_in_ht(multi_doc_ht);
        ht_clear_items(multi_doc_ht);
        LOG(LOG_INFO, "finish [%d] temp invert file", no);
    }
    fclose(fd);

    /* 最后不满一批的数据。 */
    if (count % TEMP_INVERT_DOC_SIZE != 0) {
        snprintf(path, sizeof(path), "%s/%s.%03d", invert_dir, 
                       TEMP_INVERT_NAME, no);
        dump_temp_invert(multi_doc_ht, path);
    }
    free_doc_occ_in_ht(multi_doc_ht);
    ht_clear_items(multi_doc_ht);

    /* 合并生成的临时倒排文件成一个最终的倒排 */

    return 0;
}
