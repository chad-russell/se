//
// Created by Chad Russell on 4/12/17.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include "rope.h"
#include "util.h"
#include "buf.h"

#define SPLIT_THRESHOLD 0

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

// todo(chad): this should take a (struct editor_screen_t), not a (struct rope_t *)
void
undo_stack_append(struct editor_buffer_t ctx, struct rope_t *rn)
{
    // if the circular buffer is full, free what is at the head as it will be overwritten
    if (circular_buffer_is_full(ctx.undo_buffer)) {
        struct editor_screen_t *screen = circular_buffer_at(ctx.undo_buffer, 0);

//        rope_dec_rc(screen->rn);
        rope_free(screen->rn);
    }

    SE_ASSERT(rn->rc == 0);

    struct editor_screen_t screen;
    screen.rn = rn;
    screen.cursor_info.cursor_pos = 0;
    screen.cursor_info.cursor_byte_pos = 0;
    screen.cursor_info.cursor_row = 0;
    screen.cursor_info.cursor_col = 0;
    circular_buffer_append(ctx.undo_buffer, &screen);
}

struct rope_t *
rope_new(int16_t flags, int64_t byte_weight, int64_t char_weight, struct editor_buffer_t ctx)
{
    struct rope_t *rn = se_calloc(1, sizeof(struct rope_t));

    rn->rc = 0;
    rn->flags = flags;
    rn->byte_weight = byte_weight;
    rn->char_weight = char_weight;

    return rn;
}

struct rope_t *
rope_parent_new(struct rope_t *left, struct rope_t *right, struct editor_buffer_t ctx)
{
    struct rope_t *rn = rope_new(0, rope_byte_weight(left), rope_char_weight(left), ctx);

    rope_set_left(rn, left);
    rope_set_right(rn, right);

    rn->byte_weight = rope_byte_weight(rn);
    rn->char_weight = rope_char_weight(rn);

    return rn;
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

struct rope_t *
rope_leaf_new(const char *buf, struct editor_buffer_t ctx)
{
    struct rope_t *rn = rope_new(ROPE_LEAF, (int64_t) strlen(buf), unicode_strlen(buf), ctx);
    rn->str_buf = buf_init_fmt("%str", buf);
    return rn;
}

void
rope_set_right(struct rope_t *target, struct rope_t *new_right)
{
    SE_ASSERT(target != NULL);
    SE_ASSERT(target != new_right);

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

    if (new_left == target->left) { return; }

    rope_dec_rc(target->left);
    target->left = new_left;
    rope_inc_rc(new_left);

    target->byte_weight = rope_byte_weight(target);
    target->char_weight = rope_char_weight(target);
}

struct rope_t *
rope_leaf_new_concat(const char *left, const char *right, struct editor_buffer_t ctx)
{
    struct rope_t *rn = rope_new(ROPE_LEAF,
                                           (int64_t) (strlen(left) + strlen(right)),
                                           unicode_strlen(left) + unicode_strlen(right),
                                           ctx);
    rn->str_buf = buf_init_fmt("%s", string_concat(left, right));
    return rn;
}

struct rope_t *
rope_shallow_copy(struct rope_t *rn, struct editor_buffer_t ctx)
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

void
rope_free(struct rope_t *rn)
{
    if (rn == NULL) { return; }

    if (rn->rc > 0) { return; }
    SE_ASSERT(rn->rc == 0)

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

struct rope_t *
rope_concat(struct rope_t *left, struct rope_t *right, struct editor_buffer_t ctx)
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
                return rope_leaf_new_concat(left->str_buf->bytes, right->str_buf->bytes, ctx);
            }
        }
        else if (left->right == NULL) {
            struct rope_t *cat = rope_shallow_copy(left, ctx);
            rope_set_right(cat, right);
            return cat;
        }
        else if (left->right->flags & ROPE_LEAF
                 && left->right->byte_weight + right->byte_weight < SPLIT_THRESHOLD) {
            struct rope_t *cat = rope_shallow_copy(left, ctx);
            rope_set_right(cat, rope_leaf_new_concat(left->right->str_buf->bytes, right->str_buf->bytes, ctx));
            return cat;
        }
    }

    return rope_parent_new(left, right, ctx);
}

void
rope_split_at_char(struct rope_t *rn, int64_t i,
                        struct rope_t **out_new_left,
                        struct rope_t **out_new_right,
                        struct editor_buffer_t ctx)
{
    if (rn->flags & ROPE_LEAF) {
        if (i <= 0) {
            *out_new_left = NULL;
            *out_new_right = rope_shallow_copy(rn, ctx);
        } else if (i >= rn->char_weight) {
            *out_new_left = rope_shallow_copy(rn, ctx);
            *out_new_right = NULL;
        } else {
            int32_t byte_offset = 0;
            for (int32_t j = 0; j < i; j++) {
                byte_offset += bytes_in_codepoint(*(rn->str_buf->bytes + byte_offset));
            }
            *out_new_right = rope_leaf_new(rn->str_buf->bytes + byte_offset, ctx);

            *out_new_left = rope_leaf_new(rn->str_buf->bytes, ctx);
            (*out_new_left)->byte_weight = rn->byte_weight - (*out_new_right)->byte_weight;
            (*out_new_left)->char_weight = rn->char_weight - (*out_new_right)->char_weight;
        }
    } else {
        struct rope_t *rn_copy = rope_shallow_copy(rn, ctx);
        int used_rn_copy = 0;

        if (i < rn_copy->char_weight) {
            rope_set_right(rn_copy, NULL);

            struct rope_t *new_gt_left;
            struct rope_t *new_gt_right;
            rope_split_at_char(rn_copy->left, i, &new_gt_left, &new_gt_right, ctx);

            *out_new_right = rope_concat(new_gt_right, rn->right, ctx);

            rope_set_left(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            (*out_new_left)->byte_weight = rope_byte_weight(*out_new_left);
            (*out_new_left)->char_weight = rope_char_weight(*out_new_left);
        } else if (i > rn_copy->char_weight) {
            struct rope_t *new_gt_left;
            struct rope_t *new_gt_right;
            rope_split_at_char(rn->right, i - rn_copy->char_weight, &new_gt_left, &new_gt_right, ctx);

            rope_set_right(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            *out_new_right = new_gt_right;
        } else {
            // todo(chad): why do we need to copy this?? Can we just increment the rc??
            *out_new_left = rope_shallow_copy(rn_copy->left, ctx);
            *out_new_right = rope_shallow_copy(rn_copy->right, ctx);
        }

        if (!used_rn_copy) {
            rope_free(rn_copy);
        }
    }
}

struct rope_t *
rope_insert(struct rope_t *rn, int64_t i, const char *text, struct editor_buffer_t ctx)
{
    struct rope_t *split_left;
    struct rope_t *split_right;
    rope_split_at_char(rn, i, &split_left, &split_right, ctx);

    struct rope_t *new_right = rope_leaf_new(text, ctx);

    struct rope_t *combined_left = rope_concat(split_left, new_right, ctx);
    return rope_concat(combined_left, split_right, ctx);
}

struct rope_t *
rope_append(struct rope_t *rn, const char *text, struct editor_buffer_t ctx)
{
    int64_t end = rope_total_char_length(rn);
    return rope_insert(rn, end, text, ctx);
}

struct rope_t *
rope_delete(struct rope_t *rn, int64_t start, int64_t end, struct editor_buffer_t ctx)
{
    struct rope_t *split_end_left;
    struct rope_t *split_end_right;

    rope_split_at_char(rn, end, &split_end_left, &split_end_right, ctx);

    struct rope_t *split_start_left;
    struct rope_t *split_start_right;

    rope_split_at_char(split_end_left, start, &split_start_left, &split_start_right, ctx);

    struct rope_t *result = rope_concat(split_start_left, split_end_right, ctx);

    rope_free(split_end_left);
    rope_free(split_start_right);

    return result;
}

void
rope_print(struct rope_t *rn)
{
    if (rn == NULL) {
        return;
    }

    if (rn->flags & ROPE_LEAF) {
        // todo(chad): replace this with a new buf fmt specifier
        printf("%.*s", (int) rn->byte_weight, rn->str_buf->bytes);
    } else {
        rope_print(rn->left);
        rope_print(rn->right);
    }
}

void
rope_debug_print_helper(struct rope_t *rn, int indentation)
{
    if (rn == NULL) {
        printf("%*c{NULL}\n", indentation, ' ');
        return;
    }

    if (rn->flags & ROPE_LEAF) {
        // todo(chad): replace all this with buf_t stuff
        printf("%*c{byte_weight: %lli, char_weight: %lli}[rc:%d] %.*s\n",
               indentation, ' ',
               rn->byte_weight,
               rn->char_weight,
               rn->rc,
               (int) rn->byte_weight, rn->str_buf->bytes);
    } else {
        // todo(chad): replace all this with buf_t stuff
        printf("%*c{byte_weight: %lli, char_weight: %lli}[rc:%d]\n",
               indentation,
               ' ',
               rn->byte_weight,
               rn->char_weight,
               rn->rc);

        rope_debug_print_helper(rn->left, indentation + 2);
        rope_debug_print_helper(rn->right, indentation + 2);
    }
}

void
rope_debug_print(struct rope_t *rn)
{
    rope_debug_print_helper(rn, 0);
}

void
editor_context_debug_print(struct editor_buffer_t ctx)
{
    for (int32_t i = 0; i < ctx.undo_buffer->capacity; i++) {
        struct editor_screen_t *screen_i = (struct editor_screen_t *) circular_buffer_at(ctx.undo_buffer, i);
        printf("%d:\n", i);
        rope_debug_print(screen_i->rn);
        printf("\n");
    }

}

