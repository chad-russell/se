//
// Created by Chad Russell on 8/13/17.
//

#ifndef SE_LINE_ROPE_H
#define SE_LINE_ROPE_H

#include <stdint.h>

// init
struct line_rope_t *
line_rope_parent_init(struct line_rope_t *left, struct line_rope_t *right, uint32_t virtual_line_length);

struct line_rope_t *
line_rope_leaf_init(uint32_t line_length, uint32_t virtual_line_length);

// methods
void
line_rope_set_right(struct line_rope_t *target, struct line_rope_t *new_right);

void
line_rope_set_left(struct line_rope_t *target, struct line_rope_t *new_left);

struct line_rope_t *
line_rope_char_at(struct line_rope_t *rn, int64_t i);

void
line_rope_replace_char_at(struct line_rope_t *rn, int64_t i, uint32_t new_line_length);

struct line_rope_t *
line_rope_shallow_copy(struct line_rope_t *rn);

uint32_t
line_rope_total_char_length(struct line_rope_t *rn);

uint32_t
line_rope_total_char_weight(struct line_rope_t *rn);

int64_t
line_rope_height(struct line_rope_t *rn);

uint32_t
line_rope_virtual_newline_weight(struct line_rope_t *rn);

uint32_t
line_rope_total_virtual_newline_length(struct line_rope_t *rn);

uint32_t
line_rope_total_virtual_newline_weight(struct line_rope_t *rn);

struct line_rope_t *
line_rope_insert(struct line_rope_t *rn, int64_t i, uint32_t line_length);

struct line_rope_t *
line_rope_delete(struct line_rope_t *rn, int64_t start, int64_t end);

struct line_rope_t *
line_rope_balance(struct line_rope_t *rn);

void
line_rope_inc_rc(struct line_rope_t *rn);

void
line_rope_dec_rc(struct line_rope_t *rn);

// free
void
line_rope_free(struct line_rope_t *rn);

#endif //SE_LINE_ROPE_H
