#ifndef SWL_BUF_H
#define SWL_BUF_H

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>

#include "rope.h"
#include "vector.h"
#include "util.h"

// types
struct buf_t {
    int64_t length;
    int64_t capacity;
    char *bytes;

    int32_t rc;
};

typedef void (*write_fn_t)(struct buf_t *buf, va_list list);

// init
struct buf_t *
buf_init(int64_t initial_capacity);

struct buf_t *
buf_init_fmt(const char *fmt, ...);

// methods
void
buf_ensure_free_bytes(struct buf_t *buf, int64_t capacity);

// free
void
buf_free(struct buf_t *buf);

// print
void
buf_print(struct buf_t *buf);

void
buf_print_fmt(const char *fmt, ...);

// write_fmt
struct buf_t *
buf_write_fmt(struct buf_t *buf, const char *fmt, ...);

struct buf_t *
buf_write_fmt_va(struct buf_t *buf, const char *fmt, va_list list);

int32_t
buf_write_fmt_specifier(struct buf_t *buf, const char *spec, va_list list);

// write_bytes
void
buf_write_bytes(struct buf_t *buf, const char *bytes, int64_t byte_count);

void
buf_write_bytes_va(struct buf_t *buf, va_list list);

// write_bytes_e
void
buf_write_bytes_e(struct buf_t *buf, const char *bytes, int64_t byte_count);

void
buf_write_bytes_e_va(struct buf_t *buf, va_list list);

// write_str
void
buf_write_str(struct buf_t *buf, const char *str);

void
buf_write_str_va(struct buf_t *buf, va_list list);

// write_str_une
void
buf_write_str_une(struct buf_t *buf, int64_t num_chars, const char *str);

void
buf_write_str_une_va(struct buf_t *buf, va_list list);

// write_char
void
buf_write_char(struct buf_t *buf, char c);

void
buf_write_char_va(struct buf_t *buf, va_list list);

// write_r_char
void
buf_write_r_char(struct buf_t *buf, int64_t repeat_count, char c);

void
buf_write_r_char_va(struct buf_t *buf, va_list list);

// write_i32
void
buf_write_i32(struct buf_t *buf, int32_t value);

void
buf_write_i32_va(struct buf_t *buf, va_list list);

// write_i64
void
buf_write_i64(struct buf_t *buf, int64_t value);

void
buf_write_i64_va(struct buf_t *buf, va_list list);

// write_vector
void
buf_write_vector(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator);

void
buf_write_vector_va(struct buf_t *buf, va_list list);

// write_vector_deref
void
buf_write_vector_deref(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator);

void
buf_write_vector_deref_va(struct buf_t *buf, va_list list);

// write_editor_buffer
void
buf_write_editor_buffer(struct buf_t *buf, struct editor_buffer_t *buffer);

void
buf_write_editor_buffer_va(struct buf_t *buf, va_list list);

// write_rope
void
buf_write_rope(struct buf_t *buf, struct rope_t *rn);

void
buf_write_rope_va(struct buf_t *buf, va_list list);

// write_rope_debug
void
buf_write_rope_debug(struct buf_t *buf, struct rope_t *rn);

void
buf_write_rope_debug_va(struct buf_t *buf, va_list list);

// write line_rope
void
buf_write_line_rope_va(struct buf_t *buf, va_list list);

#endif //SWL_BUF_H
