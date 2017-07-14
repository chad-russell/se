#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "rope.h"
#include "util.h"
#include "buf.h"
#include "circular_buffer.h"

#define SPLIT_THRESHOLD 100

// forward declarations
struct rope_t *
rope_new(int16_t flags, int64_t byte_weight, int64_t char_weight);

int64_t
rope_byte_weight(struct rope_t *rn);

int64_t
rope_char_weight(struct rope_t *rn);

int64_t
rope_line_break_weight(struct rope_t *rn);

int64_t
count_newlines(const char *str);

void
rope_split_at_char(struct rope_t *rn, int64_t i,
                   struct rope_t **out_new_left,
                   struct rope_t **out_new_right);

struct rope_t *
rope_concat(struct rope_t *left, struct rope_t *right);

// init
struct rope_t *
rope_parent_init(struct rope_t *left, struct rope_t *right)
{
    struct rope_t *rn = rope_new(0, rope_byte_weight(left), rope_char_weight(left));

    rope_set_left(rn, left);
    rope_set_right(rn, right);

    rn->byte_weight = rope_byte_weight(rn);
    rn->char_weight = rope_char_weight(rn);
    rn->line_break_weight = rope_line_break_weight(rn);

    return rn;
}

struct rope_t *
rope_leaf_init(const char *text)
{
    struct rope_t *rn = rope_new(ROPE_LEAF, (int64_t) strlen(text), unicode_strlen(text));
    rn->str_buf = buf_init_fmt("%str", text);
    rn->line_break_weight = count_newlines(text);
    return rn;
}

// methods
void
rope_set_right(struct rope_t *target, struct rope_t *new_right)
{
    SE_ASSERT(target != NULL);
    SE_ASSERT(target != new_right);
    SE_ASSERT(!(target->flags & ROPE_LEAF));

    if (new_right == target->right) { return; }

    rope_dec_rc(target->right);
    target->right = new_right;
    rope_inc_rc(new_right);
}

void
rope_set_left(struct rope_t *target, struct rope_t *new_left)
{
    SE_ASSERT(target != NULL);
    SE_ASSERT(target != new_left);
    SE_ASSERT(!(target->flags & ROPE_LEAF));

    if (new_left == target->left) { return; }

    rope_dec_rc(target->left);
    target->left = new_left;
    rope_inc_rc(new_left);

    target->byte_weight = rope_byte_weight(target);
    target->char_weight = rope_char_weight(target);
    target->line_break_weight = rope_line_break_weight(target);
}

const char *
rope_char_at(struct rope_t *rn, int64_t i)
{
    if (rn->flags & ROPE_LEAF) {
        if (rn->char_weight - 1 < i) {
            return NULL;
        }

        int32_t byte_offset = 0;
        for (int32_t j = 0; j < i; j++) {
            byte_offset += bytes_in_codepoint_utf8(*(rn->str_buf->bytes + byte_offset));
        }
        return rn->str_buf->bytes + byte_offset;
    }

    if (rn->char_weight - 1 < i) {
        return rope_char_at(rn->right, i - rn->char_weight);
    } else {
        if (rn->left != NULL) {
            return rope_char_at(rn->left, i);
        }
        return NULL;
    }
}

int64_t
rope_char_number_at_line(struct rope_t *rn, int64_t i)
{
    if (rn->flags & ROPE_LEAF) {
        if (rn->line_break_weight < i) {
            return 0;
        }

        int32_t byte_offset = 0;
        int64_t char_number = 0;
        for (int32_t j = 0; j < i; j++) {
            char byte_at_offset = *(rn->str_buf->bytes + byte_offset);
            while (*(rn->str_buf->bytes + byte_offset) != '\n') {
                byte_offset += bytes_in_codepoint_utf8(byte_at_offset);
                char_number += 1;
            }

            // go one further so that we skip the actual '\n' character itself
            byte_offset += bytes_in_codepoint_utf8(byte_at_offset);
            char_number += 1;
        }

        return char_number;
    }

    if (rn->line_break_weight < i) {
        int64_t left_char_length = rope_total_char_length(rn->left);
        int64_t extra = rope_char_number_at_line(rn->right, i - rn->line_break_weight);
        return left_char_length + extra;
    }
    if (rn->left != NULL) {
        return rope_char_number_at_line(rn->left, i);
    }
    return 0;
}

const char *
rope_byte_at(struct rope_t *rn, int64_t i)
{
    if (rn->flags & ROPE_LEAF) {
        if (rn->byte_weight - 1 < i) {
            return NULL;
        }
        return rn->str_buf->bytes + i;
    }

    if (rn->byte_weight - 1 < i) {
        return rope_byte_at(rn->right, i - rn->byte_weight);
    } else {
        if (rn->left != NULL) {
            return rope_byte_at(rn->left, i);
        }
        return NULL;
    }
}

int64_t
rope_total_char_length(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return rn->char_weight;
    }

    return rope_total_char_length(rn->left) + rope_total_char_length(rn->right);
}

int64_t
rope_total_line_break_length(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return rn->line_break_weight;
    }

    return rope_total_line_break_length(rn->left) + rope_total_line_break_length(rn->right);
}

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text)
{
    struct rope_t *split_left;
    struct rope_t *split_right;
    rope_split_at_char(rn, i, &split_left, &split_right);

    struct rope_t *new_right = rope_leaf_init(text);

    struct rope_t *combined_left = rope_concat(split_left, new_right);
    struct rope_t *cat = rope_concat(combined_left, split_right);

    return cat;
}

struct rope_t *
rope_delete(struct rope_t *rn, int64_t start, int64_t end)
{
    if (rope_total_char_length(rn) == 1) {
        return rope_leaf_init("");
    }

    struct rope_t *split_end_left;
    struct rope_t *split_end_right;

    rope_split_at_char(rn, end, &split_end_left, &split_end_right);

    struct rope_t *split_start_left;
    struct rope_t *split_start_right;

    rope_split_at_char(split_end_left, start, &split_start_left, &split_start_right);

    struct rope_t *result = rope_concat(split_start_left, split_end_right);

    rope_free(split_end_left);
    rope_free(split_start_right);

    return result;
}

// free
void
rope_free(struct rope_t *rn)
{
    if (rn == NULL) { return; }

    if (rn->rc > 0) { return; }
    SE_ASSERT(rn->rc == 0);

    if (!(rn->flags & ROPE_LEAF)) {
        rope_dec_rc(rn->left);
        rope_dec_rc(rn->right);
    }
    else {
        buf_free(rn->str_buf);
    }

    se_free(rn);
}

void
undo_stack_append(struct editor_buffer_t editor_buffer, struct editor_screen_t screen)
{
    // if we are overwriting something, free it first
    struct editor_screen_t *screen_to_overwrite = circular_buffer_next(editor_buffer.undo_buffer);
    if (screen_to_overwrite != NULL) {
        rope_dec_rc(screen_to_overwrite->rn);

        // nullify this so that the next time we come to it we don't try to free garbage rope data
        circular_buffer_set_next_write_index_null(editor_buffer.undo_buffer);
    }

    rope_inc_rc(screen.rn);
    circular_buffer_append(editor_buffer.undo_buffer, &screen);

    global_only_undo_stack_append(editor_buffer, screen);
}

void
global_only_undo_stack_append(struct editor_buffer_t editor_buffer, struct editor_screen_t screen)
{
    struct editor_screen_t *screen_to_overwrite = circular_buffer_next(editor_buffer.global_undo_buffer);
    if (screen_to_overwrite != NULL) {
        rope_dec_rc(screen_to_overwrite->rn);

        // nullify this so that the next time we come to it we don't try to free garbage rope data
        circular_buffer_set_next_write_index_null(editor_buffer.global_undo_buffer);
    }

    rope_inc_rc(screen.rn);
    circular_buffer_append(editor_buffer.global_undo_buffer, &screen);
}

// helpers
struct rope_t *
rope_leaf_init_concat(struct rope_t *left, struct rope_t *right)
{
    struct rope_t *rn = rope_new(ROPE_LEAF,
                                 left->byte_weight + right->byte_weight,
                                 left->char_weight + right->char_weight);
    rn->str_buf = buf_init_fmt("%str%str", left->str_buf->bytes, right->str_buf->bytes);
    rn->line_break_weight = rope_line_break_weight(rn);

    rope_free(left);
    rope_free(right);
    return rn;
}

struct rope_t *
rope_shallow_copy(struct rope_t *rn)
{
    if (rn == NULL) { return NULL; }

    struct rope_t *copy = se_alloc(1, sizeof(struct rope_t));
    memcpy(copy, rn, sizeof(struct rope_t));

    copy->rc = 0;

    if (!(rn->flags & ROPE_LEAF)) {
        rope_inc_rc(rn->left);
        rope_inc_rc(rn->right);
    } else {
        copy->str_buf = rn->str_buf;
        copy->str_buf->rc += 1;
    }

    return copy;
}

struct rope_t *
rope_new(int16_t flags, int64_t byte_weight, int64_t char_weight)
{
    struct rope_t *rn = se_alloc(1, sizeof(struct rope_t));

    rn->rc = 0;
    rn->flags = flags;
    rn->byte_weight = byte_weight;
    rn->char_weight = char_weight;

    return rn;
}

int64_t
rope_total_byte_length(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return rn->byte_weight;
    }

    return rope_total_byte_length(rn->left) + rope_total_byte_length(rn->right);
}

int64_t
rope_byte_weight(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return rn->byte_weight;
    }

    return rope_total_byte_length(rn->left);
}

int64_t
rope_char_weight(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return rn->char_weight;
    }

    return rope_total_char_length(rn->left);
}

int64_t
rope_line_break_weight(struct rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_LEAF) {
        return count_newlines(rn->str_buf->bytes);
    }

    return rope_total_line_break_length(rn->left);
}

int64_t
count_newlines(const char *str)
{
    if (str == NULL) { return 0; }
    int64_t newline_count = 0;
    for (const char *c = str; *c != 0; c++) {
        if (*c == '\n') { newline_count += 1; }
    }
    return newline_count;
}

void
rope_inc_rc(struct rope_t *rn)
{
    if (rn == NULL) { return; }

    rn->rc += 1;
}

void
rope_dec_rc(struct rope_t *rn)
{
    if (rn == NULL) { return; }

    SE_ASSERT(rn->rc > 0);

    rn->rc -= 1;
    if (rn->rc == 0) {
        rope_free(rn);
    }
}

void
rope_split_at_char(struct rope_t *rn, int64_t i,
                   struct rope_t **out_new_left,
                   struct rope_t **out_new_right)
{
    if (rn->flags & ROPE_LEAF) {
        if (i <= 0) {
            *out_new_left = NULL;
            *out_new_right = rope_shallow_copy(rn);
        } else if (i >= rn->char_weight) {
            *out_new_left = rope_shallow_copy(rn);
            *out_new_right = NULL;
        } else {
            int32_t byte_offset = 0;
            for (int32_t j = 0; j < i; j++) {
                byte_offset += bytes_in_codepoint_utf8(*(rn->str_buf->bytes + byte_offset));
            }
            *out_new_right = rope_leaf_init(rn->str_buf->bytes + byte_offset);

            struct buf_t *new_left_buf = buf_init_fmt("%bytes",
                                                      rn->str_buf->bytes,
                                                      byte_offset);
            *out_new_left = rope_leaf_init(new_left_buf->bytes);
            buf_free(new_left_buf);
        }
    } else {
        struct rope_t *rn_copy = rope_shallow_copy(rn);
        int used_rn_copy = 0;

        if (i < rn_copy->char_weight) {
            rope_set_right(rn_copy, NULL);

            struct rope_t *new_gt_left;
            struct rope_t *new_gt_right;
            rope_split_at_char(rn_copy->left, i, &new_gt_left, &new_gt_right);

            *out_new_right = rope_concat(new_gt_right, rn->right);

            rope_set_left(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            (*out_new_left)->byte_weight = rope_byte_weight(*out_new_left);
            (*out_new_left)->char_weight = rope_char_weight(*out_new_left);
            (*out_new_left)->line_break_weight = rope_line_break_weight(*out_new_left);
        } else if (i > rn_copy->char_weight) {
            struct rope_t *new_gt_left;
            struct rope_t *new_gt_right;
            rope_split_at_char(rn->right, i - rn_copy->char_weight, &new_gt_left, &new_gt_right);

            rope_set_right(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            *out_new_right = new_gt_right;
        } else {
            *out_new_left = rope_shallow_copy(rn_copy->left);
            *out_new_right = rope_shallow_copy(rn_copy->right);
        }

        if (!used_rn_copy) {
            rope_free(rn_copy);
        }
    }
}

struct rope_t *
rope_concat(struct rope_t *left, struct rope_t *right)
{
    if (right == NULL) {
        return left;
    }
    if (left == NULL) {
        return right;
    }
    if (right->flags & ROPE_LEAF) {
        if (left->flags & ROPE_LEAF) {
            if (left->byte_weight + right->byte_weight < SPLIT_THRESHOLD) {
                struct rope_t *cat = rope_leaf_init_concat(left, right);
                return cat;
            }
        }
        else if (left->right == NULL) {
            struct rope_t *cat = rope_shallow_copy(left);
            rope_set_right(cat, right);
            return cat;
        }
        else if (left->right->flags & ROPE_LEAF
                 && left->right->byte_weight + right->byte_weight < SPLIT_THRESHOLD) {
            struct rope_t *cat = rope_shallow_copy(left);
            rope_set_right(cat, rope_leaf_init_concat(left->right, right));

            rope_free(left);

            return cat;
        }
    }

    return rope_parent_init(left, right);
}
