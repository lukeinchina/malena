#ifndef MALENA_UTIL_H
#define MALENA_UTIL_H

#include <stdlib.h>
#include <stdint.h>

#include "type.h"

size_t utf8_count(const uint8_t *text, const uint8_t *end);
termid_t termid(const uint8_t *text, const uint8_t *end);
#endif
