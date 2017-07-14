#ifndef SE_STACK_H
#define SE_STACK_H

#include <stdint.h>

struct vector_t *
stack_init(int32_t item_size);

void
stack_push(struct vector_t *vector, void *item);

void *
stack_top(struct vector_t *vector);

void *
stack_top_deref(struct vector_t *vector);

void *
stack_pop(struct vector_t *vector);

void *
stack_pop_deref(struct vector_t *vector);

#endif //SE_STACK_H
