//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_ROPE_NODE_H
#define SE_ROPE_NODE_H

#include <ntsid.h>
#include "all_types.h"

struct rope_node_t *
rope_node_parent_new(struct rope_node_t *left, struct rope_node_t *right, struct editor_context_t ctx);

struct rope_node_t *
rope_node_leaf_new(const char *buf, struct editor_context_t ctx);

void
rope_node_set_right(struct rope_node_t *target, struct rope_node_t *new_right);

void
rope_node_set_left(struct rope_node_t *target, struct rope_node_t *new_left);

void
rope_node_free(struct rope_node_t *rn);

const char *
rope_node_char_at(struct rope_node_t *rn, int64_t i);

const char *
rope_node_byte_at(struct rope_node_t *rn, int64_t i);

void
editor_context_append_screen_node(struct editor_context_t ctx, struct rope_node_t *rn);

struct rope_node_t *
rope_node_insert(struct rope_node_t *rn, int64_t i, const char *text, struct editor_context_t ctx);

struct rope_node_t *
rope_node_delete(struct rope_node_t *rn, int64_t start, int64_t end, struct editor_context_t ctx);

void
rope_node_print(struct rope_node_t *rn);

void
rope_node_debug_print(struct rope_node_t *rn);

void
editor_context_debug_print(struct editor_context_t ctx);

#endif //SE_ROPE_NODE_H
