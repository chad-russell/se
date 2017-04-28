//
// Created by Chad Russell on 4/24/17.
//

#ifndef SE_CIRCULAR_BUFFER_H
#define SE_CIRCULAR_BUFFER_H

#include <stdint.h>

struct circular_buffer_t {
    void *buf;

    int32_t item_size;
    int64_t length;
    int64_t capacity;

    int64_t start;
    int64_t next_write_index;
};

struct circular_buffer_t *
circular_buffer_init(int32_t item_size, int64_t capacity);

void *
circular_buffer_at(struct circular_buffer_t *buf, int64_t index);

void *
circular_buffer_at_end(struct circular_buffer_t *buf);

void
circular_buffer_append(struct circular_buffer_t *buf, void *item);

int32_t
circular_buffer_is_full(struct circular_buffer_t *buf);

void
circular_buffer_set_next_write_index_null(struct circular_buffer_t *buf);

void
circular_buffer_truncate(struct circular_buffer_t *buf, int64_t idx);

#endif //SE_CIRCULAR_BUFFER_H
