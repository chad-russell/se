//
// Created by Chad Russell on 4/24/17.
//

#include <stddef.h>
#include <string.h>

#include "circular_buffer.h"
#include "util.h"

struct circular_buffer_t *
circular_buffer_init(int32_t item_size, int64_t capacity)
{
    struct circular_buffer_t *buf = se_calloc(1, sizeof(struct circular_buffer_t));
    buf->item_size = item_size;
    buf->capacity = capacity;
    buf->buf = se_calloc(capacity, item_size);
    buf->head = 0;
    buf->tail = 0;
    return buf;
}

void *
circular_buffer_at(struct circular_buffer_t *buf, int64_t index)
{
    int64_t real_index = (buf->head + index) % buf->capacity;

    return buf->buf + real_index * buf->item_size;
}

void
circular_buffer_append(struct circular_buffer_t *buf, void *item)
{
    if (circular_buffer_is_full(buf)) {
        buf->head = (buf->head + 1) % buf->capacity;
    }

    void *tail_ptr = buf->buf + (buf->tail * buf->item_size);
    memcpy(tail_ptr, item, buf->item_size);

    buf->tail = (buf->tail + 1) % buf->capacity;
}

int32_t
circular_buffer_is_full(struct circular_buffer_t *buf)
{
    int64_t before_head = (buf->head + buf->capacity - 1) % buf->capacity;
    return buf->tail == before_head;
}
