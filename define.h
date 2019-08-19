#ifndef MALENA_DEFINE_H
#define MALENA_DEFINE_H

/* title 最大长度。超过该长度的词不会建到索引 */
#define MAX_TITLE_LEN 256

/* 一篇文档中最多包含多少个term，超过该值的term不会建到索引中*/
#define MAX_TERM_NUM 65536

/* 多少个文档创建一个临时倒排文件*/
#define TEMP_INVERT_DOC_SIZE 1000

#define TEMP_INVERT_NAME "temp_invt"

#define INVERT_NAME "invt"

#endif
