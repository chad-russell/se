#ifndef SE_ALL_TYPES_H
#define SE_ALL_TYPES_H

#include "vector.h"

#define UNDO_BUFFER_SIZE 1000
#define GLOBAL_UNDO_BUFFER_SIZE 10000

struct rope_t {
    int64_t byte_weight;
    int64_t total_byte_weight;

    int64_t char_weight;
    int64_t total_char_weight;

    int64_t line_break_weight;
    int64_t total_line_break_weight;

    int8_t is_leaf;

    int32_t rc;

    union {
        struct {
            struct rope_t *left;
            struct rope_t *right;
        };
        struct buf_t *str_buf;
    };
};

struct line_rope_t {
    // how long is this line?
    int64_t line_length;
    int64_t total_line_length;

    // every leaf counts for one 'character'
    int64_t char_weight;
    int64_t total_char_weight;

    int32_t virtual_line_length;
    int64_t virtual_newline_count;
    int64_t total_virtual_newline_count;

    int32_t height;

    int8_t is_leaf;

    int32_t rc;

    struct line_rope_t *left;
    struct line_rope_t *right;
};

struct cursor_info_t {
    int64_t char_pos;
    int64_t row;
    int64_t col;

    int8_t is_selection;
    int64_t selection_start_char_pos;
    int64_t selection_start_row;
    int64_t selection_start_col;
};

struct editor_screen_t {
    struct cursor_info_t *cursor_info;
    struct rope_t *text;
    struct line_rope_t *lines;
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
