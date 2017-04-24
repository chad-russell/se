//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_ALL_TYPES_H
#define SE_ALL_TYPES_H

#include "vector.h"

enum ROPE_FLAGS {
    ROPE_LEAF = 1
};

struct rope_t {
    int64_t byte_weight;
    int64_t char_weight;

    int16_t flags;

    int32_t rc;

    union {
        struct {
            struct rope_t *left;
            struct rope_t *right;
        };
//        const char *buf;
        struct buf_t *str_buf;
    };
};

struct cursor_info_t {
    int64_t cursor_pos;
    int64_t cursor_byte_pos;
    int64_t cursor_row;
    int64_t cursor_col;
};

struct editor_screen_t {
    struct cursor_info_t cursor_info;
    struct rope_t *rn;
};

struct editor_buffer_t {
    struct circular_buffer_t *undo_buffer; // circular buffer of (struct editor_screen_t) roots
};

#endif //SE_ALL_TYPES_H
