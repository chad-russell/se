#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <printf.h>

#include "util.h"

// todo(chad): @Performance: make this a table lookup?
int32_t
bytes_in_codepoint_utf8(char first_byte)
{
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
//    return 1;

    SE_UNREACHABLE();
}

int64_t
unicode_strlen(const char *str)
{
    int64_t count = 0;

    const char *c = str;
    while (*c != 0) {
        int32_t bytes = bytes_in_codepoint_utf8(*c);
        for (int32_t i = 0; i < bytes; i++) {
            SE_ASSERT_MSG(c + i != NULL, "malformed utf-8!");
        }
        c += bytes;
        count += 1;
    }

    return count;
}

void *
se_alloc(int64_t count, int64_t size)
{
    void *allocated = calloc((size_t) count, (size_t) size);
    return allocated;
}

void
se_free(void *mem)
{
    free(mem);
}
