//
// Created by Chad Russell on 2/28/17.
//

#include <string.h>
#include <assert.h>
#include "vector.h"
#include "util.h"

struct vector_t *vector_init(int64_t initial_capacity, int32_t item_size) {
    struct vector_t *vec = (struct vector_t *) calloc(1, sizeof(struct vector_t));
    vec->length = 0;
    vec->capacity = initial_capacity;
    vec->item_size = item_size;
    vec->buf = calloc((size_t) initial_capacity, (size_t) item_size);
    return vec;
}

struct vector_t *vector_init_items_with_size_va(int32_t item_size, va_list list) {
    struct vector_t *vector = vector_init(1, item_size);

    void *item = va_arg(list, void *);
    while (item != NULL) {
        vector_append(vector, item);
        item = va_arg(list, void *);
    }

    return vector;
}

struct vector_t *vector_init_items_with_size(int32_t item_size, ...) {
    va_list list;
    va_start(list, item_size);

    return vector_init_items_with_size_va(item_size, list);
}

struct vector_t *vector_init_ptrsize_items(void *first_item, ...) {
    va_list list;
    va_start(list, first_item);

    struct vector_t *vector = vector_init(1, sizeof(void *));

    void *item = first_item;
    while (item != NULL) {
        vector_append(vector, item);
        item = va_arg(list, void *);
    }

    return vector;
}

void vector_grow(struct vector_t *vector) {
    assert(vector != NULL);

    int64_t new_capacity = vector->capacity * 2;

    void *new_buf = calloc((size_t) new_capacity, (size_t) vector->item_size);
    void *old_buf = vector->buf;

    memcpy(new_buf,
           vector->buf,
           (size_t) vector->capacity * vector->item_size);

    vector->capacity = new_capacity;
    vector->buf = new_buf;

    free(old_buf);
}

void *vector_at_internal(struct vector_t *vector, int64_t index) {
    return vector->buf + index * vector->item_size;
}

void vector_append(struct vector_t *vector, void *item) {
    assert(vector != NULL);

    if (vector->length == vector->capacity) {
        vector_grow(vector);
    }

    if (item != NULL) {
        memcpy(vector_at_internal(vector, vector->length),
               item,
               vector->item_size);
    }

    vector->length = vector->length + 1;
}

void *vector_at(struct vector_t *vector, int64_t index) {
    assert(vector != NULL);
    assert(index >= 0 && index < vector->length);

    return vector_at_internal(vector, index);
}

void *vector_at_deref(struct vector_t *vector, int64_t index) {
    assert(vector != NULL);

    return *((void **) vector_at(vector, index));
}

void vector_set_at(struct vector_t *vector, int64_t index, void *item) {
    assert(vector != NULL);

    memcpy(vector_at(vector, index),
           item,
           vector->item_size);
}

struct vector_t *stack_init(int32_t item_size) {
    return vector_init(1, item_size);
}

void stack_push(struct vector_t *vector, void *item) {
    assert(vector != NULL);

    vector_append(vector, item);
}

void *stack_top(struct vector_t *vector) {
    assert(vector != NULL);
    assert(vector->length > 0);

    return vector_at(vector, vector->length - 1);
}

void *stack_top_deref(struct vector_t *vector) {
    assert(vector != NULL);
    assert(vector->length > 0);

    return vector_at_deref(vector, vector->length - 1);
}

void *stack_pop(struct vector_t *vector) {
    assert(vector != NULL);
    assert(vector->length > 0);

    void *top = stack_top(vector);
    vector->length -= 1;
    return top;
}

void *stack_pop_deref(struct vector_t *vector) {
    assert(vector != NULL);
    assert(vector->length > 0);

    return *((void **) stack_pop(vector));
}
