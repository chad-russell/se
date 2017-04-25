//
// Created by Chad Russell on 4/24/17.
//

#include <assert.h>

#include "stack.h"
#include "vector.h"

struct vector_t *
stack_init(int32_t item_size)
{
    return vector_init(1, item_size);
}

void
stack_push(struct vector_t *vector, void *item)
{
    assert(vector != NULL);

    vector_append(vector, item);
}

void *
stack_top(struct vector_t *vector)
{
    assert(vector != NULL);
    assert(vector->length > 0);

    return vector_at(vector, vector->length - 1);
}

void *
stack_top_deref(struct vector_t *vector)
{
    assert(vector != NULL);
    assert(vector->length > 0);

    return vector_at_deref(vector, vector->length - 1);
}

void *
stack_pop(struct vector_t *vector)
{
    assert(vector != NULL);
    assert(vector->length > 0);

    void *top = stack_top(vector);
    vector->length -= 1;
    return top;
}

void *
stack_pop_deref(struct vector_t *vector)
{
    assert(vector != NULL);
    assert(vector->length > 0);

    return *((void **) stack_pop(vector));
}
