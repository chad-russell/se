//
// Created by Chad Russell on 4/12/17.
//


#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "util.h"

const char *
string_concat(const char *str1, const char *str2)
{
    size_t size1 = strlen(str1);
    size_t size2 = strlen(str2);

    const char *str = calloc(sizeof(char), size1 + size2);
    memcpy((void *) str, str1, size1);
    memcpy((void *) str + size1, str2, size2);
    return str;
}

int32_t
bytes_in_codepoint(char first_byte) {
    if (first_byte >= '\x00' && first_byte <= '\x7F') {
        return 1;
    }
    if (first_byte >= '\xC2' && first_byte <= '\xDF') {
        return 2;
    }
    if (first_byte >= '\xE0' && first_byte <= '\xEF') {
        return 3;
    }
    if (first_byte >= '\xF0' && first_byte <= '\xF4') {
        return 4;
    }
    SE_UNREACHABLE()
}

int64_t
unicode_strlen(const char *str)
{
    int64_t count = 0;

    const char *c = str;
    while (*c != 0) {
        int32_t bytes = bytes_in_codepoint(*c);
        for (int32_t i = 0; i < bytes; i++) {
            SE_ASSERT_MSG(c + i != NULL, "malformed utf-8!");
        }
        c += bytes;
        count += 1;
    }

    return count;
}
