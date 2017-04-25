//
// Created by Chad Russell on 4/12/17.
//

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

void
rope_inc_rc(struct rope_t *rn);

void
rope_dec_rc(struct rope_t *rn);

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

    return rn;
}

// todo(chad): @performance
// this is potentially a lot of overhead to copy this every time.
// we should probably have a convenience method to init from a buf and an offset.
// we can initialize the new buf using the bytes of the old one and increment the rc of the old one,
// the only question is how to remember to decrement the rc of the old one after the new one is freed.
// probably need a 'struct buf_t *parent' member in buf_t.
struct rope_t *
rope_leaf_init(const char *buf)
{
    struct rope_t *rn = rope_new(ROPE_LEAF, (int64_t) strlen(buf), unicode_strlen(buf));
    rn->str_buf = buf_init_fmt("%str", buf);
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
            byte_offset += bytes_in_codepoint(*(rn->str_buf->bytes + byte_offset));
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

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text)
{
    struct rope_t *split_left;
    struct rope_t *split_right;
    rope_split_at_char(rn, i, &split_left, &split_right);

    struct rope_t *new_right = rope_leaf_init(text);

    struct rope_t *combined_left = rope_concat(split_left, new_right);
    return rope_concat(combined_left, split_right);
}

struct rope_t *
rope_append(struct rope_t *rn, const char *text)
{
    int64_t end = rope_total_char_length(rn);
    return rope_insert(rn, end, text);
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

    rn->rc -= 1;

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
undo_stack_append(struct editor_buffer_t ctx, struct editor_screen_t screen)
{
    // if the circular buffer is full, free what is at the head as it will be overwritten
    if (circular_buffer_is_full(ctx.undo_buffer)) {
        struct editor_screen_t *screen_0 = circular_buffer_at(ctx.undo_buffer, 0);
        rope_free(screen_0->rn);
    }

    SE_ASSERT(screen.rn->rc == 0);
    circular_buffer_append(ctx.undo_buffer, &screen);
}

// helpers
struct rope_t *
rope_leaf_init_concat(const char *left, const char *right)
{
    struct rope_t *rn = rope_new(ROPE_LEAF,
                                 (int64_t) (strlen(left) + strlen(right)),
                                 unicode_strlen(left) + unicode_strlen(right));
    rn->str_buf = buf_init_fmt("%str%str", left, right);
    return rn;
}

struct rope_t *
rope_shallow_copy(struct rope_t *rn)
{
    if (rn == NULL) { return NULL; }

    struct rope_t *copy = se_calloc(sizeof(struct rope_t), 1);
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
    struct rope_t *rn = se_calloc(1, sizeof(struct rope_t));

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
                byte_offset += bytes_in_codepoint(*(rn->str_buf->bytes + byte_offset));
            }
            *out_new_right = rope_leaf_init(rn->str_buf->bytes + byte_offset);

            buf_print_fmt("total: %str\n", rn->str_buf->bytes);
            buf_print_fmt("right: %rope\n", *out_new_right);

            struct buf_t *new_left_buf = buf_init_fmt("%n_str",
                                                      byte_offset,
                                                      rn->str_buf->bytes);
            *out_new_left = rope_leaf_init(new_left_buf->bytes);
            buf_free(new_left_buf);

            buf_print_fmt("left: %rope\n", *out_new_left);
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
        } else if (i > rn_copy->char_weight) {
            struct rope_t *new_gt_left;
            struct rope_t *new_gt_right;
            rope_split_at_char(rn->right, i - rn_copy->char_weight, &new_gt_left, &new_gt_right);

            rope_set_right(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            *out_new_right = new_gt_right;
        } else {
            // todo(chad): why do we need to copy this?? why can't we just increment the rc??
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
                return rope_leaf_init_concat(left->str_buf->bytes, right->str_buf->bytes);
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
            rope_set_right(cat, rope_leaf_init_concat(left->right->str_buf->bytes, right->str_buf->bytes));
            return cat;
        }
    }

    return rope_parent_init(left, right);
}
