#ifndef SE_ROPE_H
#define SE_ROPE_H

#include "forward_types.h"

// init
struct rope_t *
rope_parent_init(struct rope_t *left, struct rope_t *right);

struct rope_t *
rope_leaf_init(const char *text);

struct rope_t *
rope_leaf_init_lines(const char *text, struct line_helper_t *line_helper);

struct rope_t *
rope_shallow_copy(struct rope_t *rn);

int64_t
count_newlines(const char *str);

// methods
void
rope_set_right(struct rope_t *target, struct rope_t *new_right);

void
rope_set_left(struct rope_t *target, struct rope_t *new_left);

const char *
rope_char_at(struct rope_t *rn, int64_t i);

int64_t
rope_char_number_at_line(struct rope_t *rn, int64_t i);

const char
rope_byte_at(struct rope_t *rn, int64_t i);

int64_t
byte_for_char_at(struct rope_t *rn, int64_t i);

struct rope_t *
rope_byte_at_incremental(struct rope_t *rn, struct rope_t *leaf, int64_t i, int64_t *out);

int64_t
rope_char_for_byte_at(struct rope_t *rn, int64_t i);

int64_t
rope_total_char_length(struct rope_t *rn);

int64_t
rope_total_char_weight(struct rope_t *rn);

int64_t
rope_total_byte_length(struct rope_t *rn);

int64_t
rope_total_byte_weight(struct rope_t *rn);

int64_t
rope_total_line_break_length(struct rope_t *rn);

int64_t
rope_total_line_break_weight(struct rope_t *rn);

int64_t
rope_get_line_number_for_char_pos(struct rope_t *rn, int64_t char_pos);

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text);

struct rope_t *
rope_delete(struct rope_t *rn, int64_t start, int64_t end);

void
rope_inc_rc(struct rope_t *rn);

void
rope_dec_rc(struct rope_t *rn);

// free
void
rope_free(struct rope_t *rn);

// undo stack
void
global_only_undo_stack_append(struct editor_buffer_t editor_buffer, struct editor_screen_t screen);

void
undo_stack_append(struct editor_buffer_t editor_buffer, struct editor_screen_t screen);

#endif //SE_ROPE_H
