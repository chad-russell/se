
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
    struct editor_buffer_t editor_buffer = editor_buffer_create(80);

//    editor_buffer_open_file(editor_buffer, 80, "/Users/chadrussell/Projects/Clion/se/todo.txt");
//    int64_t found = editor_buffer_search_forward(editor_buffer, "macros", 0);

    editor_buffer_open_file(editor_buffer, 80, "/Users/chadrussell/Desktop/macros.txt");
    int64_t found = editor_buffer_search_forward(editor_buffer, "macros", 0);

    buf_print_fmt("%i64\n", found);
    return 0;
}
