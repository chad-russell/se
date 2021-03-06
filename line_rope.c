//
// Created by Chad Russell on 8/13/17.
//

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "line_rope.h"
#include "util.h"
#include "buf.h"
#include "circular_buffer.h"
#include "stack.h"

// forward declarations
struct line_rope_t *
line_rope_new(int8_t is_leaf, int64_t virtual_line_length);

uint32_t
line_rope_line_length(struct line_rope_t *rn);

uint32_t
line_rope_char_weight(struct line_rope_t *rn);

void
line_rope_split_at_char(struct line_rope_t *rn, int64_t i,
                   struct line_rope_t **out_new_left,
                   struct line_rope_t **out_new_right);

struct line_rope_t *
line_rope_concat(struct line_rope_t *left, struct line_rope_t *right);

uint32_t
line_rope_total_line_length(struct line_rope_t *rn);

uint32_t
line_rope_longest_child_line_length(struct line_rope_t *rn);

// init
struct line_rope_t *
line_rope_new(int8_t is_leaf, int64_t virtual_line_length)
{
    line_rope_id += 1;

    struct line_rope_t *rn = se_alloc(1, sizeof(struct line_rope_t));

    rn->rc = 0;
    rn->is_leaf = is_leaf;
    rn->virtual_line_length = (uint32_t) virtual_line_length;

    return rn;
}

struct line_rope_t *
line_rope_parent_init(struct line_rope_t *left, struct line_rope_t *right, int64_t virtual_line_length)
{
    struct line_rope_t *rn = line_rope_new(0, virtual_line_length);

    line_rope_set_left(rn, left);
    line_rope_set_right(rn, right);

    rn->line_length = line_rope_line_length(rn);
    rn->total_line_length = line_rope_total_line_length(rn);
    rn->longest_child_line_length = line_rope_longest_child_line_length(rn);

    rn->char_weight = line_rope_char_weight(rn);
    rn->total_char_weight = line_rope_total_char_weight(rn);

    rn->virtual_newline_count = line_rope_virtual_newline_weight(rn);
    rn->total_virtual_newline_count = line_rope_total_virtual_newline_weight(rn);

    line_rope_height(rn);

    return rn;
}

struct line_rope_t *
line_rope_leaf_init(uint32_t line_length, uint32_t virtual_line_length)
{
    struct line_rope_t *rn = line_rope_new(1, virtual_line_length);

    rn->line_length = line_length;
    rn->total_line_length = line_length;
    rn->longest_child_line_length = line_length;

    rn->char_weight = 1;
    rn->total_char_weight = 1;

    rn->height = 0;

    rn->virtual_newline_count = line_rope_virtual_newline_weight(rn);
    rn->total_virtual_newline_count = rn->virtual_newline_count;

    return rn;
}

// methods
void
line_rope_set_right(struct line_rope_t *target, struct line_rope_t *new_right)
{
    SE_ASSERT(target != NULL);
    SE_ASSERT(target != new_right);
    SE_ASSERT(!target->is_leaf);

    if (new_right == target->right) { return; }

    line_rope_dec_rc(target->right);
    target->right = new_right;
    line_rope_inc_rc(new_right);

    line_rope_height(target);

    target->line_length = line_rope_line_length(target);
    line_rope_total_line_length(target);
    line_rope_longest_child_line_length(target);

    target->char_weight = line_rope_char_weight(target);
    line_rope_total_char_weight(target);

    target->virtual_newline_count = line_rope_virtual_newline_weight(target);
    line_rope_total_virtual_newline_weight(target);
}

void
line_rope_set_left(struct line_rope_t *target, struct line_rope_t *new_left)
{
    SE_ASSERT(target != NULL);
    SE_ASSERT(target != new_left);
    SE_ASSERT(!target->is_leaf);

    if (new_left == target->left) { return; }

    line_rope_dec_rc(target->left);
    target->left = new_left;
    line_rope_inc_rc(new_left);

    line_rope_height(target);

    target->line_length = line_rope_line_length(target);
    line_rope_total_line_length(target);
    line_rope_longest_child_line_length(target);

    target->char_weight = line_rope_char_weight(target);
    line_rope_total_char_weight(target);

    target->virtual_newline_count = line_rope_virtual_newline_weight(target);
    line_rope_total_virtual_newline_weight(target);
}

struct line_rope_t *
line_rope_char_at(struct line_rope_t *rn, int64_t i)
{
    if (rn == NULL) { return NULL; }

    if (rn->is_leaf) {
        if ((int64_t) rn->char_weight - 1 < i) {
            return NULL;
        }

        return rn;
    }

    if ((int64_t) rn->char_weight - 1 < i) {
        return line_rope_char_at(rn->right, i - rn->char_weight);
    } else {
        if (rn->left != NULL) {
            return line_rope_char_at(rn->left, i);
        }
        return NULL;
    }
}

int64_t
line_rope_char_number_at_line_helper(struct line_rope_t *rn, int64_t i)
{
    if (rn == NULL) { return 0; }
    if (i == 0) { return 0; }

    if (rn->is_leaf) {
        return 0;
    }

    if ((int64_t) rn->char_weight - 1 < i) {
        int64_t left_weight = rn->left == NULL ? 0 : rn->left->total_line_length;
        return left_weight + line_rope_char_number_at_line_helper(rn->right, i - rn->char_weight);
    } else {
        if (rn->left != NULL) {
            return line_rope_char_number_at_line_helper(rn->left, i);
        }

        return 0;
    }
}

int64_t
line_rope_char_number_at_line(struct line_rope_t *rn, int64_t i)
{
    if (rn == NULL) { return 0; }
    if (i == 0) { return 0; }

    return i + line_rope_char_number_at_line_helper(rn, i);
}

struct line_rope_t *
line_rope_replace_char_at(struct line_rope_t *rn, int64_t i, int64_t new_line_length)
{
    if (rn == NULL) { return NULL; }

    if (rn->is_leaf) {
        if ((int64_t) rn->char_weight - 1 < i) {
            return NULL;
        }

        struct line_rope_t *new_line = line_rope_leaf_init((uint32_t) new_line_length, rn->virtual_line_length);
        return new_line;
    }

    if ((int64_t) rn->char_weight - 1 < i) {
        if (rn->right != NULL) {
            struct line_rope_t *new_right = line_rope_replace_char_at(rn->right, i - rn->char_weight, new_line_length);
            struct line_rope_t *copy = line_rope_shallow_copy(rn);
            line_rope_set_right(copy, new_right);
            return copy;
        }
        return NULL;
    } else {
        if (rn->left != NULL) {
            struct line_rope_t *new_left = line_rope_replace_char_at(rn->left, i, new_line_length);
            struct line_rope_t *copy = line_rope_shallow_copy(rn);
            line_rope_set_left(copy, new_left);
            return copy;
        }
        return NULL;
    }
}


uint32_t
line_rope_total_char_length(struct line_rope_t *rn)
{
    if (rn == NULL) { return 0; }

    return rn->total_char_weight;
}

uint32_t
line_rope_total_char_weight(struct line_rope_t *rn)
{
    if (rn == NULL) { return 0; }

    if (rn->left != NULL && rn->right != NULL) {
        rn->total_char_weight = rn->left->total_char_weight + rn->right->total_char_weight;
    }
    else if (rn->left != NULL) {
        rn->total_char_weight = rn->left->total_char_weight;
    } else if (rn->right != NULL) {
        rn->total_char_weight = rn->right->total_char_weight;
    } else {
        rn->total_char_weight = 0;
    }

    return rn->total_char_weight;
}

int64_t
line_rope_height(struct line_rope_t *rn)
{
    struct line_rope_t *left = rn->left;
    struct line_rope_t *right = rn->right;

    if (left == NULL && right == NULL) {
        rn->height = 0;
    } else if (left == NULL) {
        rn->height = right->height + 1;
    } else if (right == NULL) {
        rn->height = left->height + 1;
    } else if (left->height > right->height) {
        rn->height = left->height + 1;
    } else {
        rn->height = right->height + 1;
    }

    return rn->height;
}

uint32_t
line_rope_total_virtual_newline_length(struct line_rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    return rn->total_virtual_newline_count;
}

uint32_t
line_rope_total_virtual_newline_weight(struct line_rope_t *rn)
{
    if (rn == NULL) { return 0; }

    else if (rn->is_leaf) {
        return rn->virtual_newline_count;
    }

    if (rn->left != NULL && rn->right != NULL) {
        rn->total_virtual_newline_count = rn->left->total_virtual_newline_count + rn->right->total_virtual_newline_count;
    }
    else if (rn->left != NULL) {
        rn->total_virtual_newline_count = rn->left->total_virtual_newline_count;
    } else if (rn->right != NULL) {
        rn->total_virtual_newline_count = rn->right->total_virtual_newline_count;
    } else {
        rn->total_virtual_newline_count = 0;
    }

    return rn->total_virtual_newline_count;
}

void
line_rope_balance_helper(struct line_rope_t *rn, struct vector_t *stack)
{
    if (rn == NULL) { return; }

    if (rn->is_leaf) {
        stack_push(stack, &rn);

        int8_t c = 0;
        while (stack->length > 1 && !c) {
            struct line_rope_t *top = (struct line_rope_t *) vector_at_deref(stack, stack->length - 1);
            struct line_rope_t *second = (struct line_rope_t *) vector_at_deref(stack, stack->length - 2);
            if (top->height == second->height) {
                stack_pop(stack);
                stack_pop(stack);

//                struct line_rope_t *new_line = line_rope_parent_init(second, top, rn->virtual_line_length);
//                stack_push(stack, &new_line);
            } else {
                c = 1;
            }
        }
    } else {
        line_rope_balance_helper(rn->left, stack);
        line_rope_balance_helper(rn->right, stack);
    }
}

struct line_rope_t *
line_rope_balance(struct line_rope_t *rn)
{
    if (rn == NULL) { return NULL; }

    struct vector_t *stack = stack_init(sizeof(struct line_rope_t *));

    line_rope_balance_helper(rn, stack);
    while (stack->length > 1) {
        struct line_rope_t *top = (struct line_rope_t *) vector_at_deref(stack, stack->length - 1);
        struct line_rope_t *second = (struct line_rope_t *) vector_at_deref(stack, stack->length - 2);
        stack_pop(stack);
        stack_pop(stack);

        struct line_rope_t *new_line = line_rope_parent_init(second, top, rn->virtual_line_length);
        stack_push(stack, &new_line);
    }
    SE_ASSERT(stack->length == 1);

    struct line_rope_t *top = (struct line_rope_t *) stack_top_deref(stack);

    vector_free(stack);
    return top;
}

struct line_rope_t *
line_rope_insert(struct line_rope_t *rn, int64_t i, int64_t line_length)
{
    if (rn == NULL) { return NULL; }

    struct line_rope_t *split_left;
    struct line_rope_t *split_right;
    line_rope_split_at_char(rn, i, &split_left, &split_right);

//    SE_ASSERT(split_left->total_char_weight == i);
//    SE_ASSERT(split_right->total_char_weight == rn->total_char_weight - i);

    struct line_rope_t *insert = line_rope_leaf_init((uint32_t) line_length, rn->virtual_line_length);

    struct line_rope_t *combined_left = line_rope_concat(split_left, insert);
    struct line_rope_t *cat = line_rope_concat(combined_left, split_right);

    return cat;
}

struct line_rope_t *
line_rope_delete(struct line_rope_t *rn, int64_t start, int64_t end)
{
    if (rn == NULL) { return NULL; }

    if (line_rope_total_char_length(rn) == 1) {
        return line_rope_leaf_init(1, rn->virtual_line_length);
    }

    struct line_rope_t *split_end_left;
    struct line_rope_t *split_end_right;

    line_rope_split_at_char(rn, end, &split_end_left, &split_end_right);

    struct line_rope_t *split_start_left;
    struct line_rope_t *split_start_right;

    line_rope_split_at_char(split_end_left, start, &split_start_left, &split_start_right);

    struct line_rope_t *result = line_rope_concat(split_start_left, split_end_right);

    line_rope_free(split_end_left);
    line_rope_free(split_start_right);

    return result;
}

// free
void
line_rope_free(struct line_rope_t *rn)
{
    if (rn == NULL) { return; }

    SE_ASSERT(rn->rc == 0);

    if (!rn->is_leaf) {
        line_rope_dec_rc(rn->left);
        line_rope_dec_rc(rn->right);
    }

    se_free(rn);
}

// helpers
struct line_rope_t *
line_rope_shallow_copy(struct line_rope_t *rn)
{
    if (rn == NULL) { return NULL; }

    struct line_rope_t *copy = se_alloc(1, sizeof(struct line_rope_t));
    memcpy(copy, rn, sizeof(struct line_rope_t));

    copy->rc = 0;

    if (!rn->is_leaf) {
        line_rope_inc_rc(rn->left);
        line_rope_inc_rc(rn->right);
    }

    return copy;
}

uint32_t
line_rope_total_line_length(struct line_rope_t *rn)
{
    if (rn == NULL) { return 0; }

    if (rn->left != NULL && rn->right != NULL) {
        rn->total_line_length = rn->left->total_line_length + rn->right->total_line_length;
    } else if (rn->left != NULL) {
        rn->total_line_length = rn->left->total_line_length;
    } else if (rn->right != NULL) {
        rn->total_line_length = rn->right->total_line_length;
    } else {
        rn->total_line_length = rn->line_length;
    }

    return rn->total_line_length;
}

uint32_t
line_rope_longest_child_line_length(struct line_rope_t *rn)
{
    if (rn == NULL) { return 0; }

    if (rn->left != NULL && rn->right != NULL) {
        rn->longest_child_line_length = rn->left->longest_child_line_length >= rn->right->longest_child_line_length ?
                                        rn->left->longest_child_line_length :
                                        rn->right->longest_child_line_length;
    }
    else if (rn->left != NULL) {
        rn->longest_child_line_length = rn->left->longest_child_line_length;
    } else if (rn->right != NULL) {
        rn->longest_child_line_length = rn->right->longest_child_line_length;
    } else {
        rn->longest_child_line_length = rn->line_length;
    }

    return rn->longest_child_line_length;
}

uint32_t
line_rope_line_length(struct line_rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->is_leaf) {
        return rn->line_length;
    }

    return line_rope_total_line_length(rn->left);
}

uint32_t
line_rope_char_weight(struct line_rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->is_leaf) {
        return rn->char_weight;
    }

    return line_rope_total_char_length(rn->left);
}

uint32_t
line_rope_virtual_newline_weight(struct line_rope_t *rn)
{
    if (rn == NULL) {
        return 0;
    }

    if (rn->is_leaf) {
        uint32_t line_length = rn->line_length;
        uint32_t virtual_line_count = 0;

        if (line_length == 0) {
            virtual_line_count += 1;
        } else {
            virtual_line_count = line_length / rn->virtual_line_length;

            int64_t remainder = line_length % rn->virtual_line_length;
            if (remainder != 0) {
                virtual_line_count += 1;
            }
        }

        return virtual_line_count;
    }

    return line_rope_total_virtual_newline_length(rn->left);
}

void
line_rope_inc_rc(struct line_rope_t *rn)
{
    if (rn == NULL) { return; }

    rn->rc += 1;
}

void
line_rope_dec_rc(struct line_rope_t *rn)
{
    if (rn == NULL) { return; }

    SE_ASSERT(rn->rc > 0);

    rn->rc -= 1;
    if (rn->rc == 0) {
        line_rope_free(rn);
    }
}

void
line_rope_split_at_char(struct line_rope_t *rn, int64_t i,
                   struct line_rope_t **out_new_left,
                   struct line_rope_t **out_new_right)
{
    if (rn == NULL) {
        *out_new_left = NULL;
        *out_new_right = NULL;
        return;
    }

    if (rn->is_leaf) {
        if (i <= 0) {
            *out_new_left = NULL;
            *out_new_right = line_rope_shallow_copy(rn);
        } else if (i >= rn->char_weight) {
            *out_new_left = line_rope_shallow_copy(rn);
            *out_new_right = NULL;
        } else {
            SE_ASSERT_MSG(0, "with a char_weight of 1 for every leaf node, we should never reach this case");
        }
    } else {
        struct line_rope_t *rn_copy = line_rope_shallow_copy(rn);
        int used_rn_copy = 0;

        if (i < rn_copy->char_weight) {
            line_rope_set_right(rn_copy, NULL);

            struct line_rope_t *new_gt_left;
            struct line_rope_t *new_gt_right;
            line_rope_split_at_char(rn_copy->left, i, &new_gt_left, &new_gt_right);

            *out_new_right = line_rope_concat(new_gt_right, rn->right);

            line_rope_set_left(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            (*out_new_left)->line_length = line_rope_line_length(*out_new_left);
            (*out_new_left)->total_line_length = line_rope_total_line_length(*out_new_left);
            (*out_new_left)->longest_child_line_length = line_rope_longest_child_line_length(*out_new_left);

            (*out_new_left)->char_weight = line_rope_char_weight(*out_new_left);
            (*out_new_left)->total_char_weight = line_rope_total_char_weight(*out_new_left);

            (*out_new_left)->virtual_newline_count = line_rope_virtual_newline_weight(*out_new_left);
            (*out_new_left)->total_virtual_newline_count = line_rope_total_virtual_newline_weight(*out_new_left);
        } else if (i > rn_copy->char_weight) {
            struct line_rope_t *new_gt_left;
            struct line_rope_t *new_gt_right;
            line_rope_split_at_char(rn->right, i - rn_copy->char_weight, &new_gt_left, &new_gt_right);

            line_rope_set_right(rn_copy, new_gt_left);

            *out_new_left = rn_copy;
            used_rn_copy = 1;

            *out_new_right = new_gt_right;
        } else {
            *out_new_left = line_rope_shallow_copy(rn_copy->left);
            *out_new_right = line_rope_shallow_copy(rn_copy->right);
        }

        if (!used_rn_copy) {
            line_rope_free(rn_copy);
        }
    }
}

struct line_rope_t *
line_rope_concat(struct line_rope_t *left, struct line_rope_t *right)
{
    if (right == NULL) {
        return left;
    }
    if (left == NULL) {
        return right;
    }
    if (right->is_leaf && !left->is_leaf && left->right == NULL) {
        struct line_rope_t *cat = line_rope_shallow_copy(left);
        line_rope_set_right(cat, right);
        return cat;
    }

    return line_rope_parent_init(left, right, left->virtual_line_length);
}
