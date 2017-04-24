//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_rope_H
#define SE_rope_H

#include <ntsid.h>
#include "all_types.h"

struct rope_t *
rope_parent_new(struct rope_t *left, struct rope_t *right, struct editor_buffer_t ctx);

struct rope_t *
rope_leaf_new(const char *buf, struct editor_buffer_t ctx);

void
rope_set_right(struct rope_t *target, struct rope_t *new_right);

void
rope_set_left(struct rope_t *target, struct rope_t *new_left);

void
rope_dec_rc(struct rope_t *rn);

void
rope_free(struct rope_t *rn);

const char *
rope_char_at(struct rope_t *rn, int64_t i);

const char *
rope_byte_at(struct rope_t *rn, int64_t i);

int64_t
rope_total_char_length(struct rope_t *rn);

void
undo_stack_append(struct editor_buffer_t ctx, struct rope_t *rn);

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text, struct editor_buffer_t ctx);

struct rope_t *
rope_append(struct rope_t *rn, const char *text, struct editor_buffer_t ctx);

struct rope_t *
rope_delete(struct rope_t *rn, int64_t start, int64_t end, struct editor_buffer_t ctx);

void
rope_inc_rc(struct rope_t *rn);

void
rope_print(struct rope_t *rn);

void
rope_debug_print(struct rope_t *rn);

void
editor_context_debug_print(struct editor_buffer_t ctx);

#endif //SE_rope_H
