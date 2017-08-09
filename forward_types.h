#ifndef SE_ALL_TYPES_H
#define SE_ALL_TYPES_H

#include "vector.h"

#define UNDO_BUFFER_SIZE 1000
#define GLOBAL_UNDO_BUFFER_SIZE 10000

enum ROPE_FLAGS {
    ROPE_LEAF = 1
};

struct rope_t {
    int64_t byte_weight;
    int64_t char_weight;
    int64_t line_break_weight;

    int16_t flags;

    int32_t rc;

    union {
        struct {
            struct rope_t *left;
            struct rope_t *right;
        };
        struct buf_t *str_buf;
    };
};

struct cursor_info_t {
    int64_t char_pos;
    int64_t row;
    int64_t col;
};

struct editor_screen_t {
    struct cursor_info_t cursor_info;
    struct vector_t *line_lengths;
    struct rope_t *rn;
};

struct editor_buffer_t {
    struct buf_t *file_path;

    struct circular_buffer_t *global_undo_buffer; // circular buffer of (struct editor_screen_t)
    struct circular_buffer_t *undo_buffer; // circular buffer of (struct editor_screen_t)

    int64_t *undo_idx;
    int64_t *global_undo_idx;

    struct editor_screen_t *current_screen;
};

#endif //SE_ALL_TYPES_H
