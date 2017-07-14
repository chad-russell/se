#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "buf.h"

// init
struct buf_t *
buf_init(int64_t initial_capacity)
{
    struct buf_t *buf = (struct buf_t *) se_alloc(1, sizeof(struct buf_t));
    buf->bytes = (char *) se_alloc(initial_capacity + 1, sizeof(char));
    buf->length = 0;
    buf->capacity = initial_capacity + 1;
    buf->rc = 0;
    return buf;
}

struct buf_t *
buf_init_fmt(const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);

    return buf_write_fmt_va(NULL, fmt, list);
}

// helper
void
buf_ensure_free_bytes(struct buf_t *buf, int64_t capacity)
{
    int realloc = 0;
    while (buf->length + capacity >= buf->capacity - 1) {
        realloc = 1;
        buf->capacity *= 2;
    }
    if (realloc) {
        char *old_bytes = buf->bytes;
        buf->bytes = (char *) se_alloc(1, buf->capacity);
        memcpy(buf->bytes, old_bytes, buf->length * sizeof(char));
        free(old_bytes);
    }
}

// free
void
buf_free(struct buf_t *buf)
{
    if (buf == NULL) { return; }

    if (buf->rc > 0) {
        buf->rc -= 1;
    } else {
        free(buf->bytes);
        free(buf);
    }
}

// print
void
buf_print(struct buf_t *buf)
{
    buf_print_fmt("%str", buf->bytes);
}

void
buf_print_fmt(const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);

    struct buf_t *buf = buf_write_fmt_va(NULL, fmt, list);
    printf("%s", buf->bytes);
    buf_free(buf);
}

// write_fmt
struct buf_t *
buf_write_fmt(struct buf_t *buf, const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);

    return buf_write_fmt_va(buf, fmt, list);
}

struct buf_t *
buf_write_fmt_va(struct buf_t *buf, const char *fmt, va_list list)
{
    if (buf == NULL) {
        unsigned long min_init_len = strlen(fmt) + 512;
        int64_t init_len;
        for (init_len = 16; init_len < min_init_len; init_len *= 2) { }
        buf = buf_init(init_len);
    }

    int idx = 0;
    while (fmt[idx] != '\0') {
        if (fmt[idx] == '%') {
            idx += 1;
            idx += buf_write_fmt_specifier(buf, fmt + idx, list);
        }
        else {
            buf_write_char(buf, fmt[idx]);
            idx += 1;
        }
    }

    return buf;
}

int
buf_write_fmt_specifier_helper(struct buf_t *buf, const char *src, const char *spec, write_fn_t write_fn, va_list list)
{
    if (strncmp(src, spec, strlen(spec)) == 0) {
        write_fn(buf, list);
        return 1;
    }
    return 0;
}

int32_t
buf_write_fmt_specifier(struct buf_t *buf, const char *src, va_list list)
{
    if (buf_write_fmt_specifier_helper(buf, src, "bytes_e", buf_write_bytes_e_va, list)) {
        return (int32_t) strlen("bytes_e");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "bytes", buf_write_bytes_va, list)) {
        return (int32_t) strlen("bytes");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "str_une", buf_write_str_une_va, list)) {
        return (int32_t) strlen("str_une");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "str", buf_write_str_va, list)) {
        return (int32_t) strlen("str");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "char", buf_write_char_va, list)) {
        return (int32_t) strlen("char");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "r_char", buf_write_r_char_va, list)) {
        return (int32_t) strlen("r_char");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "i32", buf_write_i32_va, list)) {
        return (int32_t) strlen("i32");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "i64", buf_write_i64_va, list)) {
        return (int32_t) strlen("i64");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "vector_deref", buf_write_vector_deref_va, list)) {
        return (int32_t) strlen("vector_deref");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "vector", buf_write_vector_va, list)) {
        return (int32_t) strlen("vector");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "rope_debug", buf_write_rope_debug_va, list)) {
        return (int32_t) strlen("rope_debug");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "rope", buf_write_rope_va, list)) {
        return (int32_t) strlen("rope");
    }

    SE_PANIC("unknown specifier");
    exit(-1);
}

// write_bytes_e
void
buf_write_bytes_e(struct buf_t *buf, const char *bytes, int64_t byte_count)
{
    for (int64_t i = 0; i < byte_count; i++) {
        if (bytes[i] == '\n') {
            buf_write_char(buf, '\\');
            buf_write_char(buf, 'n');
        } else if (bytes[i] == '\r') {
            buf_write_char(buf, '\\');
            buf_write_char(buf, 'r');
        } else if (bytes[i] == '\t') {
            buf_write_char(buf, '\\');
            buf_write_char(buf, 't');
        } else if (bytes[i] == '"') {
            buf_write_char(buf, '\\');
            buf_write_char(buf, '"');
        } else if (bytes[i] == '\\') {
            buf_write_char(buf, '\\');
            buf_write_char(buf, '\\');
        } else {
            buf_write_char(buf, bytes[i]);
        }
    }
}

void
buf_write_bytes_e_va(struct buf_t *buf, va_list list)
{
    buf_write_bytes_e(buf, va_arg(list, const char *), va_arg(list, int64_t));
}

// write_bytes
void
buf_write_bytes(struct buf_t *buf, const char *bytes, int64_t byte_count)
{
    buf_ensure_free_bytes(buf, byte_count);

    char *bytes_ptr = buf->bytes;
    memcpy(bytes_ptr + buf->length, bytes, byte_count);
    buf->length += byte_count;
}

void
buf_write_bytes_va(struct buf_t *buf, va_list list)
{
    buf_write_bytes(buf, va_arg(list, const char *), va_arg(list, int64_t));
}

// write_str
void
buf_write_str(struct buf_t *buf, const char *str)
{
    int64_t byte_count = (int64_t) strlen(str);
    buf_write_bytes(buf, str, byte_count);
}

void
buf_write_str_va(struct buf_t *buf, va_list list)
{
    buf_write_str(buf, va_arg(list, const char *));
}

// write_str_une
void
buf_write_str_une(struct buf_t *buf, int64_t num_chars, const char *str)
{
    for (int64_t i = 0; i < num_chars;) {
        if (str[i] == '\\') {
            i += 1;
            if (i < num_chars) {
                switch (str[i]) {
                    case 'n': {
                        buf_write_char(buf, '\n');
                    } break;
                    case 'r': {
                        buf_write_char(buf, '\r');
                    } break;
                    case 't': {
                        buf_write_char(buf, '\t');
                    } break;
                    case '"': {
                        buf_write_char(buf, '"');
                    } break;
                    case '\\': {
                        buf_write_char(buf, '\\');
                    } break;
                    default:break;
                }
            }
        } else {
            buf_write_char(buf, str[i]);
        }
        i += 1;
    }
}

void
buf_write_str_une_va(struct buf_t *buf, va_list list)
{
    buf_write_str_une(buf, va_arg(list, int64_t), va_arg(list, const char *));
}

// write_char
void
buf_write_char(struct buf_t *buf, char c)
{
    buf_ensure_free_bytes(buf, 1);

    buf->bytes[buf->length] = c;
    buf->length += 1;
}

void
buf_write_char_va(struct buf_t *buf, va_list list)
{
    buf_write_char(buf, va_arg(list, int));
}

// write_r_char
void
buf_write_r_char(struct buf_t *buf, int64_t repeat_count, char c)
{
    for (int64_t i = 0; i < repeat_count; i++) {
        buf_write_char(buf, c);
    }
}

void
buf_write_r_char_va(struct buf_t *buf, va_list list)
{
    buf_write_r_char(buf, va_arg(list, int64_t), va_arg(list, int));
}

// write_i32
void
buf_write_i32(struct buf_t *buf, int32_t value)
{
    int64_t size = snprintf(NULL, 0, "%i", value);
    char *s = (char *) se_alloc(size + 1, sizeof(char));
    sprintf(s, "%i", value);

    buf_write_str(buf, s);

    se_free(s);
}

void
buf_write_i32_va(struct buf_t *buf, va_list list)
{
    buf_write_i32(buf, va_arg(list, int32_t));
}

// write_i64
void
buf_write_i64(struct buf_t *buf, int64_t value)
{
    int64_t size = snprintf(NULL, 0, "%lli", value);
    char *s = (char *) se_alloc(size + 1, sizeof(char));
    sprintf(s, "%lli", value);

    buf_write_str(buf, s);

    free(s);
}

void
buf_write_i64_va(struct buf_t *buf, va_list list)
{
    buf_write_i64(buf, va_arg(list, int64_t));
}

// write_vector
void
buf_write_vector(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator)
{
    if (vector->length == 0) { return; }

    buf_write_fmt(buf, elem_fmt, vector_at(vector, 0));
    for (int64_t i = 1; i < vector->length; i++) {
        buf_write_fmt(buf, separator);
        buf_write_fmt(buf, elem_fmt, vector_at(vector, i));
    }
}

void
buf_write_vector_va(struct buf_t *buf, va_list list)
{
    buf_write_vector(buf, va_arg(list, const char *), va_arg(list, struct vector_t *), va_arg(list, const char *));
}

// write_vector_deref
void
buf_write_vector_deref(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator)
{
    if (vector->length == 0) { return; }

    buf_write_fmt(buf, elem_fmt, vector_at_deref(vector, 0));
    for (int64_t i = 1; i < vector->length; i++) {
        buf_write_fmt(buf, separator);
        buf_write_fmt(buf, elem_fmt, vector_at_deref(vector, (int64_t) i));
    }
}

void
buf_write_vector_deref_va(struct buf_t *buf, va_list list)
{
    buf_write_vector_deref(buf, va_arg(list, const char *), va_arg(list, struct vector_t *), va_arg(list, const char *));
}

// write_rope
void
buf_write_rope(struct buf_t *buf, struct rope_t *rn)
{
    if (rn == NULL) {
        return;
    }

    if (rn->flags & ROPE_LEAF) {
        buf_write_bytes(buf, rn->str_buf->bytes, rn->byte_weight);
        return;
    }

    buf_write_rope(buf, rn->left);
    buf_write_rope(buf, rn->right);
}

void
buf_write_rope_va(struct buf_t *buf, va_list list)
{
    buf_write_rope(buf, va_arg(list, struct rope_t *));
}

// write_rope_debug
void
buf_write_rope_debug_helper(struct buf_t *buf, struct rope_t *rn, int32_t indent)
{
    if (rn == NULL) {
        buf_write_fmt(buf, "%r_char{NULL}\n", indent, ' ');
        return;
    }

    if (rn->flags & ROPE_LEAF) {
        buf_write_fmt(buf, "%r_char{byte_weight: %i64, char_weight: %i64}[rc:%i32] %bytes_e\n",
                      indent, ' ',
                      rn->byte_weight,
                      rn->char_weight,
                      rn->rc,
                      rn->str_buf->bytes, rn->byte_weight);
    } else {
        buf_write_fmt(buf, "%r_char{byte_weight: %i64, char_weight: %i64}[rc:%i32]\n",
                      indent, ' ',
                      rn->byte_weight,
                      rn->char_weight,
                      rn->rc);

        buf_write_rope_debug_helper(buf, rn->left, indent+ 2);
        buf_write_rope_debug_helper(buf, rn->right, indent+ 2);
    }
}

void
buf_write_rope_debug(struct buf_t *buf, struct rope_t *rn)
{
    buf_write_rope_debug_helper(buf, rn, 0);
}

void
buf_write_rope_debug_va(struct buf_t *buf, va_list list)
{
    buf_write_rope_debug(buf, va_arg(list, struct rope_t *));
}

