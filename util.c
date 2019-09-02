#include <string.h>

#include "util.h"
#include "md5.h"

size_t 
utf8_count(const uint8_t *text, const uint8_t *end) {
    size_t count = 0;

    while (text < end) {
        if (0X00 == (*text & 0X80)) {
            text += 1;
        } else if (0XC0 == (*text & 0XE0)) {
            text += 2;
        } else if (0XE0 == (*text & 0XF0)) {
            text += 3;
        } else {
            text += 4;
        }
		count += 1;
    }

    return count;
}

/*
 *
 */
termid_t 
termid(const uint8_t *text, const uint8_t *end) {
	unsigned char digest[16];
	termid_t id;
	MD5_CTX ctx;
    md5_init(&ctx);
    md5_update(&ctx, text, end - text);
    md5_final(digest, &ctx);
	memcpy(&id, digest, sizeof(id));
	return id;
}


/* ---------------------------------------------------------- */
