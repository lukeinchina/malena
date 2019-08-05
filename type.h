#ifndef MALENA_TYPE_H
#define MALENA_TYPE_H

#include <stdint.h>

/* 假设单个索引分片内文档数目不超过42亿 */
typedef uint32_t docid_t;

/* 一个语义单元(即常说的词) 称为 term. termid即词的hash值 */
typedef uint64_t termid_t; 

/* term出现的位置. (occurrence), 也就是说最多有64k个字符 */
typedef uint16_t occ_t;

#endif
