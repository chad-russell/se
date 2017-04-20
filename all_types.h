//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_ALL_TYPES_H
#define SE_ALL_TYPES_H

#include "vector.h"

enum ROPE_NODE_FLAGS {
    ROPE_NODE_LEAF = 1,
    ROPE_NODE_HIGHLIGHT = 1 << 1,
    ROPE_NODE_BUF_NEEDS_FREE = 1 << 2
};

struct rope_node_t {
    int64_t byte_weight;
    int64_t char_weight;

    int16_t flags;
    int32_t rc;

    union {
        struct {
            struct rope_node_t *left;
            struct rope_node_t *right;
        };
        const char *buf;
    };
};

struct editor_screen_t {
    int64_t cursor_pos;
    int64_t cursor_byte_pos;
    int64_t cursor_row;
    int64_t cursor_col;

    struct rope_node_t *root;
};

struct editor_context_t {
    struct vector_t *undo_stack; // stack of (struct editor_screen_t) roots
};

#endif //SE_ALL_TYPES_H
