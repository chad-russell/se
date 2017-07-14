#ifndef SE_UTIL_H
#define SE_UTIL_H

#include <assert.h>

#define SE_ASSERT(cond) assert(cond)
#define SE_ASSERT_MSG(cond, msg) SE_ASSERT(cond && msg)
#define SE_PANIC(msg) SE_ASSERT_MSG(0, msg)
#define SE_UNREACHABLE() SE_PANIC("unreachable"); exit(1)
#define SE_TODO() SE_PANIC("todo"); exit(1)

int
bytes_in_codepoint_utf8(char first_byte);

int64_t
unicode_strlen(const char *str);

void *
se_alloc(int64_t count, int64_t size);

void
se_free(void *mem);

#endif //SE_UTIL_H
