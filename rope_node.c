//
// Created by Chad Russell on 4/12/17.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <printf.h>

#include "rope_node.h"
#include "util.h"

#define SPLIT_THRESHOLD 0

int64_t
rope_node_byte_weight(struct rope_node_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_NODE_LEAF) {
        return rn->byte_weight;
    }

    return rope_node_byte_weight(rn->left) + rope_node_byte_weight(rn->right);
}

int64_t
rope_node_char_weight(struct rope_node_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->flags & ROPE_NODE_LEAF) {
        return rn->char_weight;
    }

    return rope_node_char_weight(rn->left) + rope_node_char_weight(rn->right);
}

void
editor_context_append_screen_node(struct editor_context_t ctx, struct rope_node_t *rn)
{
    if (ctx.undo_stack->length > 0) {
        struct editor_screen_t *last_screen = (struct editor_screen_t *) vector_at(ctx.undo_stack,
                                                                                   ctx.undo_stack->length - 1);
        struct editor_screen_t new_screen = *last_screen;
        new_screen.root = rn;
        vector_append(ctx.undo_stack, &new_screen);
    } else {
        struct editor_screen_t new_screen;
        new_screen.root = rn;
        vector_append(ctx.undo_stack, &new_screen);
    }
}

struct rope_node_t *
rope_node_new(int16_t flags, int64_t byte_weight, int64_t char_weight, struct editor_context_t ctx)
{
    struct rope_node_t *rn = calloc(1, sizeof(struct rope_node_t));

    rn->rc = 0;
    rn->flags = flags;
    rn->byte_weight = byte_weight;
    rn->char_weight = char_weight;

    return rn;
}

struct rope_node_t *
rope_node_parent_new(struct rope_node_t *left, struct rope_node_t *right, struct editor_context_t ctx)
{
    struct rope_node_t *rn = rope_node_new(0, rope_node_byte_weight(left), rope_node_char_weight(left), ctx);

    rope_node_set_left(rn, left);
    rope_node_set_right(rn, right);

    return rn;
}

void
rope_node_dec_rc(struct rope_node_t *rn)
{
    if (rn == NULL) { return; }

    rn->rc -= 1;
    if (rn->rc == 0) {
        rope_node_free(rn);
    }
}

struct rope_node_t *
rope_node_leaf_new(const char *buf, struct editor_context_t ctx)
{
    struct rope_node_t *rn = rope_node_new(ROPE_NODE_LEAF, (int64_t) strlen(buf), unicode_strlen(buf), ctx);
    rn->buf = buf;

    return rn;
}

void
rope_node_set_right(struct rope_node_t *target, struct rope_node_t *new_right)
{
    assert(target != NULL);

    rope_node_dec_rc(target->right);

    target->right = new_right;
    if (new_right != NULL) {
        new_right->rc += 1;
    }
}

void
rope_node_set_left(struct rope_node_t *target, struct rope_node_t *new_left)
{
    assert(target != NULL);

    rope_node_dec_rc(target->left);

    target->left = new_left;
    if (new_left != NULL) {
        new_left->rc += 1;
    }
}

struct rope_node_t *
rope_node_leaf_new_concat(const char *left, const char *right, struct editor_context_t ctx)
{
    struct rope_node_t *rn = rope_node_new(ROPE_NODE_LEAF,
                                           (int64_t) (strlen(left) + strlen(right)),
                                           unicode_strlen(left) + unicode_strlen(right),
                                           ctx);
    rn->buf = string_concat(left, right);
    rn->flags |= ROPE_NODE_BUF_NEEDS_FREE;
    return rn;
}

struct rope_node_t *
rope_node_shallow_copy(struct rope_node_t *rn, struct editor_context_t ctx)
{
    struct rope_node_t *copy = calloc(sizeof(struct rope_node_t), 1);
    memcpy(copy, rn, sizeof(struct rope_node_t));

    if (!(rn->flags & ROPE_NODE_LEAF)) {
        if (rn->left != NULL) {
            rn->left->rc += 1;
        }
        if (rn->right != NULL) {
            rn->right->rc += 1;
        }
    }

    editor_context_append_screen_node(ctx, copy);

    return copy;
}

void
rope_node_free(struct rope_node_t *rn)
{
    if (rn == NULL) { return; }

    if (rn->rc > 0) { return; }

    if (!(rn->flags & ROPE_NODE_LEAF)) {
        rope_node_dec_rc(rn->left);
        rope_node_dec_rc(rn->right);
    }
    else if (rn->flags & ROPE_NODE_BUF_NEEDS_FREE) {
        free((void *) rn->buf);
    }

    // @debug
//    memset(rn, 0, sizeof(struct rope_node_t));
//    rn->buf = "***DELETED***";

    free(rn);
}

const char *
rope_node_byte_at(struct rope_node_t *rn, int64_t i)
{
    if (rn->flags & ROPE_NODE_LEAF) {
        if (rn->byte_weight - 1 < i) {
            return NULL;
        }
        return rn->buf + i;
    }

    if (rn->byte_weight - 1 < i) {
        return rope_node_byte_at(rn->right, i - rn->byte_weight);
    } else {
        if (rn->left != NULL) {
            return rope_node_byte_at(rn->left, i);
        }
        return NULL;
    }
}

const char *
rope_node_char_at(struct rope_node_t *rn, int64_t i)
{
    if (rn->flags & ROPE_NODE_LEAF) {
        if (rn->char_weight - 1 < i) {
            return NULL;
        }

        int32_t byte_offset = 0;
        for (int32_t j = 0; j < i; j++) {
            byte_offset += bytes_in_codepoint(*(rn->buf + byte_offset));
        }
        return rn->buf + byte_offset;
    }

    if (rn->char_weight - 1 < i) {
        return rope_node_char_at(rn->right, i - rn->char_weight);
    } else {
        if (rn->left != NULL) {
            return rope_node_char_at(rn->left, i);
        }
        return NULL;
    }
}

struct rope_node_t *
rope_node_concat(struct rope_node_t *left, struct rope_node_t *right, struct editor_context_t ctx)
{
    if (right == NULL) {
        return left;
    }
    if (left == NULL) {
        return right;
    }
    if (right->flags & ROPE_NODE_LEAF) {
        if (left->flags & ROPE_NODE_LEAF) {
            if (left->byte_weight + right->byte_weight < SPLIT_THRESHOLD) {
                return rope_node_leaf_new_concat(left->buf, right->buf, ctx);
            }
        }
        else if (left->right == NULL) {
            struct rope_node_t *cat = rope_node_shallow_copy(left, ctx);
            rope_node_set_right(cat, right);
            return cat;
        }
        else if (left->right->flags & ROPE_NODE_LEAF
                 && left->right->byte_weight + right->byte_weight < SPLIT_THRESHOLD) {
            struct rope_node_t *cat = rope_node_shallow_copy(left, ctx);
            rope_node_set_right(cat, rope_node_leaf_new_concat(left->right->buf, right->buf, ctx));
            return cat;
        }
    }

    return rope_node_parent_new(left, right, ctx);
}

void
rope_node_split_at_char(struct rope_node_t *rn, int64_t i,
                        struct rope_node_t **out_new_left,
                        struct rope_node_t **out_new_right,
                        struct editor_context_t ctx)
{
    if (rn->flags & ROPE_NODE_LEAF) {
        if (i <= 0) {
            *out_new_left = NULL;
            *out_new_right = rope_node_shallow_copy(rn, ctx);
        } else if (i >= rn->char_weight) {
            *out_new_left = rope_node_shallow_copy(rn, ctx);
            *out_new_right = NULL;
        } else {
            int32_t byte_offset = 0;
            for (int32_t j = 0; j < i; j++) {
                byte_offset += bytes_in_codepoint(*(rn->buf + byte_offset));
            }
            *out_new_right = rope_node_leaf_new(rn->buf + byte_offset, ctx);

            *out_new_left = rope_node_leaf_new(rn->buf, ctx);
            (*out_new_left)->byte_weight = rn->byte_weight - (*out_new_right)->byte_weight;
            (*out_new_left)->char_weight = rn->char_weight - (*out_new_right)->char_weight;
        }
    } else {
        struct rope_node_t *rn_copy = rope_node_shallow_copy(rn, ctx);

        if (i < rn_copy->char_weight) {
            rope_node_set_right(rn_copy, NULL);

            struct rope_node_t *new_gt_left;
            struct rope_node_t *new_gt_right;
            rope_node_split_at_char(rn_copy->left, i, &new_gt_left, &new_gt_right, ctx);

            *out_new_right = rope_node_concat(new_gt_right, rn->right, ctx);

            rope_node_set_left(rn_copy, new_gt_left);
            *out_new_left = rn_copy;
            (*out_new_left)->byte_weight = rn->byte_weight - (*out_new_right)->byte_weight;
            (*out_new_left)->char_weight = rn->char_weight - (*out_new_right)->char_weight;
        } else if (i > rn_copy->char_weight) {
            struct rope_node_t *new_gt_left;
            struct rope_node_t *new_gt_right;
            rope_node_split_at_char(rn->right, i - rn_copy->char_weight, &new_gt_left, &new_gt_right, ctx);

            rope_node_set_right(rn_copy, new_gt_left);
            *out_new_left = rn_copy;
            *out_new_right = new_gt_right;
        } else {
            *out_new_left = rope_node_shallow_copy(rn_copy->left, ctx);
            *out_new_right = rope_node_shallow_copy(rn_copy->right, ctx);
        }
    }
}

struct rope_node_t *
rope_node_insert(struct rope_node_t *rn, int64_t i, const char *text, struct editor_context_t ctx)
{
    struct rope_node_t *split_left;
    struct rope_node_t *split_right;
    rope_node_split_at_char(rn, i, &split_left, &split_right, ctx);

    struct rope_node_t *combined_left = rope_node_concat(split_left, rope_node_leaf_new(text, ctx), ctx);
    return rope_node_concat(combined_left, split_right, ctx);
}

struct rope_node_t *
rope_node_delete(struct rope_node_t *rn, int64_t start, int64_t end, struct editor_context_t ctx)
{
    struct rope_node_t *split_end_left;
    struct rope_node_t *split_end_right;

    rope_node_split_at_char(rn, end, &split_end_left, &split_end_right, ctx);

    struct rope_node_t *split_start_left;
    struct rope_node_t *split_start_right;

    rope_node_split_at_char(split_end_left, start, &split_start_left, &split_start_right, ctx);

    struct rope_node_t *result = rope_node_concat(split_start_left, split_end_right, ctx);

    rope_node_free(split_end_left);
    rope_node_free(split_end_right);
    rope_node_free(split_start_left);
    rope_node_free(split_start_right);

    return result;
}

void
rope_node_print(struct rope_node_t *rn)
{
    if (rn == NULL) {
        return;
    }

    if (rn->flags & ROPE_NODE_LEAF) {
        printf("%.*s", (int) rn->byte_weight, rn->buf);
    } else {
        rope_node_print(rn->left);
        rope_node_print(rn->right);
    }
}

void
rope_node_debug_print_helper(struct rope_node_t *rn, int indentation)
{
    if (rn == NULL) {
        printf("%*c{NULL}\n", indentation, ' ');
        return;
    }

    if (rn->flags & ROPE_NODE_LEAF) {
        printf("%*c{weight: %lli}[rc:%d] %.*s\n", indentation, ' ', rn->byte_weight, rn->rc, (int) rn->byte_weight, rn->buf);
    } else {
        printf("%*c{weight: %lli}[rc:%d]\n", indentation, ' ', rn->byte_weight, rn->rc);
        rope_node_debug_print_helper(rn->left, indentation + 2);
        rope_node_debug_print_helper(rn->right, indentation + 2);
    }
}

void
rope_node_debug_print(struct rope_node_t *rn)
{
    rope_node_debug_print_helper(rn, 0);
}

void
editor_context_debug_print(struct editor_context_t ctx)
{
    for (int32_t i = 0; i < ctx.undo_stack->length; i++) {
        struct editor_screen_t *screen_i = vector_at(ctx.undo_stack, i);
        printf("%d:\n", i);
        rope_node_debug_print(screen_i->root);
        printf("\n");
    }

}

