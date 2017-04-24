//
// Created by Chad Russell on 11/8/16.
//

#ifndef SWL_BUF_H
#define SWL_BUF_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#include "rope.h"
#include "vector.h"
#include "util.h"

struct buf_t {
    size_t length;
    size_t capacity;
    char *bytes;

    int32_t rc;

    // formatting stuff
    int32_t indent;
};

typedef void (*write_fn_t)(struct buf_t *buf, va_list list);

struct buf_t *buf_init(size_t initial_capacity);
struct buf_t *buf_init_fmt(const char *fmt, ...);

void buf_free(struct buf_t *buf);

struct buf_t *buf_write_fmt(struct buf_t *buf, const char *fmt, ...);
struct buf_t *buf_write_fmt_va(struct buf_t *buf, const char *fmt, va_list list);

void buf_print(struct buf_t *buf);
void buf_print_fmt(const char *fmt, ...);

void buf_ensure_free_bytes(struct buf_t *buf, int64_t capacity);
void buf_write_bytes(struct buf_t *buf, const char *bytes, int64_t byte_count);

void buf_write_str(struct buf_t *buf, const char *str);
void buf_write_str_va(struct buf_t *buf, va_list list);

void buf_write_char(struct buf_t *buf, char c);
void buf_write_char_va(struct buf_t *buf, va_list list);

void buf_write_i32(struct buf_t *buf, int32_t value);
void buf_write_i32_va(struct buf_t *buf, va_list list);

void buf_write_i64(struct buf_t *buf, int64_t value);
void buf_write_i64_va(struct buf_t *buf, va_list list);

void buf_write_vector(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator);
void buf_write_vector_va(struct buf_t *buf, va_list list);

void buf_write_vector_deref(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator);
void buf_write_vector_deref_va(struct buf_t *buf, va_list list);

void buf_write_rope(struct buf_t *buf, struct rope_t *rn);
void buf_write_rope_va(struct buf_t *buf, va_list list);

int32_t buf_write_fmt_specifier(struct buf_t *buf, const char *spec, va_list list);

#endif //SWL_BUF_H
