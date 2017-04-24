//
// Created by Chad Russell on 2/28/17.
//

#ifndef SWL_VECTOR_H
#define SWL_VECTOR_H

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

struct vector_t {
    int64_t length;
    int64_t capacity;

    int32_t item_size;

    char *buf;
};

struct vector_t *vector_init(int64_t initial_capacity, int32_t item_size);
struct vector_t *vector_init_items_with_size_va(int32_t item_size, va_list list);
struct vector_t *vector_init_items_with_size(int32_t item_size, ...);
struct vector_t *vector_init_ptrsize_items(void *first_item, ...);
void vector_grow(struct vector_t *vector);
void vector_append(struct vector_t *vector, void *item);
void *vector_at(struct vector_t *vector, int64_t index);
void *vector_at_deref(struct vector_t *vector, int64_t index);
void vector_set_at(struct vector_t *vector, int64_t index, void *item);

struct vector_t *stack_init(int32_t item_size);
void stack_push(struct vector_t *vector, void *item);
void *stack_top(struct vector_t *vector);
void *stack_top_deref(struct vector_t *vector);
void *stack_pop(struct vector_t *vector);
void *stack_pop_deref(struct vector_t *vector);

struct circular_buffer_t {
    void *buf;

    int32_t item_size;
    int64_t capacity;

    int64_t head;
    int64_t tail;
};

struct circular_buffer_t *circular_buffer_init(int32_t item_size, int64_t capacity);
void *circular_buffer_at(struct circular_buffer_t *buf, int64_t index);
void circular_buffer_append(struct circular_buffer_t *buf, void *item);
int32_t circular_buffer_is_full(struct circular_buffer_t *buf);

#endif //SWL_VECTOR_H
