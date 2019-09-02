#ifndef MALENA_MERGE_H
#define MALENA_MERGE_H

/* 64 bytes */
struct TempInvertHead {
    size_t termid_num;
    size_t others[7];
};

struct InvertHead {
    size_t termid_num;
    size_t others[7];
};

/*
 * @brief:
 *        (1) 合并多个临时倒排后，一个termid所对应的docid和位置信息。
 *        统一存储，然后写入最终的倒排文件中。
 *
 *        (2) 检索时，需要从倒排文件中读取termid的倒排，也用该结构存储。
 */
typedef struct {
    termid_t termid;
    float    idf;

    uint32_t  doc_size;
    uint32_t  doc_num;
    docid_t  *doclist;
    /* 记录每个docid 位置记录的开始地址/偏移量 */
    uint32_t *occ_offsets;

    /* 位置记录,格式为{{occ_num}{occ}+ }+ */
    uint32_t  occ_byte;
    uint32_t  occ_len;
    uint8_t  *occ_buff;   

}TermInvCell;

int merge_temp_inverts(char **temp_inv_paths, size_t size, const char *invert_dir);

#endif
