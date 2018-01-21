
#include <string.h>
#include "forward_types.h"
#include "buf.h"
#include "editor_buffer.h"

void print_buffer(struct editor_buffer_t buffer) {
    int64_t line_count = editor_buffer_get_line_count(buffer);
    for (int64_t i = 0; i < line_count; i++) {
        struct buf_t *line_buf = editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0);
        buf_print_fmt("%str", line_buf->bytes);
    }
}

int
main()
{
    return 0;
}
