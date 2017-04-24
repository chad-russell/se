//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_UTIL_H
#define SE_UTIL_H

#include <assert.h>

#define SE_ASSERT(cond) assert(cond);
#define SE_ASSERT_MSG(cond, msg) assert(cond && msg);
#define SE_PANIC(msg) SE_ASSERT_MSG(0, msg)
#define SE_UNREACHABLE() SE_PANIC("unreachable") exit(1);
#define SE_TODO() SE_PANIC("todo") exit(1);

const char *
string_concat(const char *str1, const char *str2);

int
bytes_in_codepoint(char first_byte);

int64_t
unicode_strlen(const char *str);

void *
se_calloc(size_t count, size_t size);

void
se_free(void *mem);

#endif //SE_UTIL_H
