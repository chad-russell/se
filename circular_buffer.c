#include <string.h>

#include "circular_buffer.h"
#include "util.h"
#include "buf.h"

struct circular_buffer_t *
circular_buffer_init(int32_t item_size, int64_t capacity)
{
    struct circular_buffer_t *buf = se_alloc(1, sizeof(struct circular_buffer_t));
    buf->item_size = item_size;
    buf->capacity = capacity;
    buf->buf = se_alloc(capacity, item_size);
    buf->start = 0;
    buf->next_write_index = 0;
    buf->length = 0;
    return buf;
}

void *
circular_buffer_at(struct circular_buffer_t *buf, int64_t index)
{
    int64_t real_index = (buf->start + index) % buf->capacity;
    return buf->buf + real_index * buf->item_size;
}

void *
circular_buffer_at_end(struct circular_buffer_t *buf)
{
    return circular_buffer_at(buf, buf->length - 1);
}

void *
circular_buffer_next(struct circular_buffer_t *buf)
{
    return buf->buf + (buf->next_write_index * buf->item_size);
}

void
circular_buffer_append(struct circular_buffer_t *buf, void *item)
{
    if (circular_buffer_is_full(buf)) {
        buf->start = (buf->start + 1) % buf->capacity;
    } else {
        buf->length += 1;
    };

    void *write_ptr = buf->buf + (buf->next_write_index * buf->item_size);
    memcpy(write_ptr, item, buf->item_size);

    buf->next_write_index = (buf->next_write_index + 1) % buf->capacity;
}

int32_t
circular_buffer_is_full(struct circular_buffer_t *buf)
{
    return buf->length == buf->capacity;
}

void
circular_buffer_set_next_write_index_null(struct circular_buffer_t *buf)
{
    memset(buf->buf + buf->next_write_index * buf->item_size, 0, buf->item_size);
}

void
circular_buffer_truncate(struct circular_buffer_t *buf, int64_t idx)
{
    buf->length = idx + 1;
    buf->next_write_index = (buf->start + idx + 1) % buf->capacity;
}
