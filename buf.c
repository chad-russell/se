//
// Created by Chad Russell on 11/8/16.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "buf.h"

struct buf_t *buf_write_fmt_va(struct buf_t *buf, const char *fmt, va_list list) {
    if (buf == NULL) {
        unsigned long min_init_len = strlen(fmt) + 16;
        size_t init_len;
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

struct buf_t *buf_init_fmt(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    return buf_write_fmt_va(NULL, fmt, list);
}

void buf_print_fmt(const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    struct buf_t *buf = buf_write_fmt_va(NULL, fmt, list);
    printf("%s", buf->bytes);
    buf_free(buf);
}

void buf_print(struct buf_t *buf) {
    buf_print_fmt("%str", buf->bytes);
}

struct buf_t *buf_write_fmt(struct buf_t *buf, const char *fmt, ...) {
    va_list list;
    va_start(list, fmt);

    return buf_write_fmt_va(buf, fmt, list);
}

struct buf_t *buf_init(size_t initial_capacity) {
    struct buf_t *buf = (struct buf_t *) se_calloc(sizeof(char), sizeof(struct buf_t));
    buf->bytes = (char *) se_calloc(initial_capacity + 1, sizeof(char));
    buf->length = 0;
    buf->capacity = initial_capacity + 1;
    buf->rc = 0;
    return buf;
}

void buf_free(struct buf_t *buf) {
    if (buf->rc > 0) {
        buf->rc -= 1;
    } else {
        free(buf->bytes);
        free(buf);
    }
}

void buf_ensure_free_bytes(struct buf_t *buf, int64_t capacity) {
    int realloc = 0;
    while (buf->length + capacity >= buf->capacity - 1) {
        realloc = 1;
        buf->capacity *= 2;
    }
    if (realloc) {
        char *old_bytes = buf->bytes;
        buf->bytes = (char *) se_calloc(1, buf->capacity);
        memcpy(buf->bytes, old_bytes, buf->length * sizeof(char));
        free(old_bytes);
    }
}

void buf_write_str(struct buf_t *buf, const char *str) {
    int64_t byte_count = (int64_t) strlen(str);
    buf_write_bytes(buf, str, byte_count);
}

void buf_write_str_va(struct buf_t *buf, va_list list) {
    buf_write_str(buf, va_arg(list, const char *));
}

void buf_write_char(struct buf_t *buf, char c) {
    buf_ensure_free_bytes(buf, 1);

    buf->bytes[buf->length] = c;
    buf->length += 1;
}

void buf_write_char_va(struct buf_t *buf, va_list list) {
    buf_write_char(buf, va_arg(list, int));
}

void buf_write_bytes(struct buf_t *buf, const char *bytes, int64_t byte_count) {
    buf_ensure_free_bytes(buf, byte_count);

    char *bytes_ptr = buf->bytes;
    memcpy(bytes_ptr + buf->length, bytes, byte_count);
    buf->length += byte_count;
}

void buf_write_i32(struct buf_t *buf, int32_t value) {
    size_t size = (size_t) snprintf(NULL, 0, "%i", value);
    char *s = (char *) se_calloc(size + 1, sizeof(char));
    sprintf(s, "%i", value);

    buf_write_str(buf, s);

    se_free(s);
}

void buf_write_i32_va(struct buf_t *buf, va_list list) {
    buf_write_i32(buf, va_arg(list, int32_t));
}

void buf_write_i64(struct buf_t *buf, int64_t value) {
    size_t size = (size_t) snprintf(NULL, 0, "%lli", value);
    char *s = (char *) se_calloc(size + 1, sizeof(char));
    sprintf(s, "%lli", value);

    buf_write_str(buf, s);

    free(s);
}

void buf_write_i64_va(struct buf_t *buf, va_list list) {
    buf_write_i64(buf, va_arg(list, int64_t));
}

void buf_write_vector(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator) {
    if (vector->length == 0) { return; }

    buf_write_fmt(buf, elem_fmt, vector_at(vector, 0));
    for (size_t i = 1; i < vector->length; i++) {
        buf_write_fmt(buf, separator);
        buf_write_fmt(buf, elem_fmt, vector_at(vector, i));
    }
}

void buf_write_vector_va(struct buf_t *buf, va_list list) {
    buf_write_vector(buf, va_arg(list, const char *), va_arg(list, struct vector_t *), va_arg(list, const char *));
}

void buf_write_vector_deref(struct buf_t *buf, const char *elem_fmt, struct vector_t *vector, const char *separator) {
    if (vector->length == 0) { return; }

    buf_write_fmt(buf, elem_fmt, vector_at_deref(vector, 0));
    for (size_t i = 1; i < vector->length; i++) {
        buf_write_fmt(buf, separator);
        buf_write_fmt(buf, elem_fmt, vector_at_deref(vector, (int64_t) i));
    }
}

void buf_write_vector_deref_va(struct buf_t *buf, va_list list) {
    buf_write_vector_deref(buf, va_arg(list, const char *), va_arg(list, struct vector_t *), va_arg(list, const char *));
}

void buf_write_rope(struct buf_t *buf, struct rope_t *rn) {
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

void buf_write_rope_va(struct buf_t *buf, va_list list) {
    buf_write_rope(buf, va_arg(list, struct rope_t *));
}

int buf_write_fmt_specifier_helper(struct buf_t *buf, const char *src, const char *spec, write_fn_t write_fn, va_list list) {
    if (strncmp(src, spec, strlen(spec)) == 0) {
        write_fn(buf, list);
        return 1;
    }
    return 0;
}

int32_t buf_write_fmt_specifier(struct buf_t *buf, const char *src, va_list list) {
    if (buf_write_fmt_specifier_helper(buf, src, "str", buf_write_str_va, list)) {
        return (int32_t) strlen("str");
    }
    if (buf_write_fmt_specifier_helper(buf, src, "char", buf_write_char_va, list)) {
        return (int32_t) strlen("char");
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
    if (buf_write_fmt_specifier_helper(buf, src, "rope", buf_write_rope_va, list)) {
        return (int32_t) strlen("rope");
    }

    SE_PANIC("unknown specifier")
    exit(-1);
}

