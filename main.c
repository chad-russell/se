
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

int64_t
editor_buffer_get_virtual_cursor_row_for_point(struct editor_buffer_t editor_buffer,
                                               int64_t row,
                                               int64_t col,
                                               int64_t virtual_line_length);

int
main()
{
    struct editor_buffer_t buffer = editor_buffer_create();

//    editor_buffer_open_file(buffer, "/Users/chadrussell/Desktop/shakespeare.txt");
    editor_buffer_open_file(buffer, "/Users/chadrussell/Desktop/hello.txt");

//    for (int i = 0; i < 10; i++) {
//        struct buf_t *line = editor_buffer_get_text_between_points_virtual(buffer, i, 0, i + 1, 0, 10);
//        buf_print_fmt("%str\n", line->bytes);
//    }

    editor_buffer_get_virtual_cursor_row_for_point(buffer, 10, 0, 10);


    return 0;
}
