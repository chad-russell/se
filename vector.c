//
// Created by Chad Russell on 2/28/17.
//

#include <string.h>
#include <assert.h>
#include "vector.h"
#include "util.h"

struct vector_t *vector_init(int64_t initial_capacity, int32_t item_size) {
    struct vector_t *vec = (struct vector_t *) se_calloc(1, sizeof(struct vector_t));
    vec->length = 0;
    vec->capacity = initial_capacity;
    vec->item_size = item_size;
    vec->buf = se_calloc((size_t) initial_capacity, (size_t) item_size);
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

    void *new_buf = se_calloc((size_t) new_capacity, (size_t) vector->item_size);
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

struct circular_buffer_t *circular_buffer_init(int32_t item_size, int64_t capacity) {
    struct circular_buffer_t *buf = se_calloc(1, sizeof(struct circular_buffer_t));
    buf->item_size = item_size;
    buf->capacity = capacity;
    buf->buf = se_calloc((size_t) capacity, (size_t) item_size);
    buf->head = 0;
    buf->tail = 0;
    return buf;
}

void *circular_buffer_at(struct circular_buffer_t *buf, int64_t index) {
    int64_t real_index = (buf->head + index) % buf->capacity;

    return buf->buf + real_index * buf->item_size;
}

void circular_buffer_append(struct circular_buffer_t *buf, void *item) {
    if (circular_buffer_is_full(buf)) {
        buf->head = (buf->head + 1) % buf->capacity;
    }

    void *tail_ptr = buf->buf + (buf->tail * buf->item_size);
    memcpy(tail_ptr, item, buf->item_size);

    buf->tail = (buf->tail + 1) % buf->capacity;
}

int32_t circular_buffer_is_full(struct circular_buffer_t *buf) {
    int64_t before_head = (buf->head + buf->capacity - 1) % buf->capacity;
    return buf->tail == before_head;
}
