//
// Created by Chad Russell on 4/12/17.
//

#ifndef SE_rope_H
#define SE_rope_H

#include <ntsid.h>
#include "forward_types.h"

// init
struct rope_t *
rope_parent_init(struct rope_t *left, struct rope_t *right);

struct rope_t *
rope_leaf_init(const char *text);

// methods
void
rope_set_right(struct rope_t *target, struct rope_t *new_right);

void
rope_set_left(struct rope_t *target, struct rope_t *new_left);

const char *
rope_char_at(struct rope_t *rn, int64_t i);

const char *
rope_byte_at(struct rope_t *rn, int64_t i);

int64_t
rope_total_char_length(struct rope_t *rn);

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text);

struct rope_t *
rope_append(struct rope_t *rn, const char *text);

struct rope_t *
rope_delete(struct rope_t *rn, int64_t start, int64_t end);

// free
void
rope_free(struct rope_t *rn);

void
undo_stack_append(struct editor_buffer_t ctx, struct editor_screen_t screen);

#endif //SE_rope_H
