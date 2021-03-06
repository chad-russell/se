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

// init
struct vector_t *
vector_init(int64_t initial_capacity, int32_t item_size);

struct vector_t *
vector_init_items_with_size_va(int32_t item_size, va_list list);

struct vector_t *
vector_init_items_with_size(int32_t item_size, ...);

struct vector_t *
vector_init_ptrsize_items(void *first_item, ...);

struct vector_t *
vector_copy(struct vector_t *orig);

// methods
void
vector_grow(struct vector_t *vector);

void
vector_append(struct vector_t *vector, void *item);

void *
vector_at(struct vector_t *vector, int64_t index);

void *
vector_at_deref(struct vector_t *vector, int64_t index);

void
vector_set_at(struct vector_t *vector, int64_t index, void *item);

// free
void
vector_free(struct vector_t *vector);

#endif //SWL_VECTOR_H
