
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

int
main()
{
    struct editor_buffer_t buffer = editor_buffer_create(30);

//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/very_large.txt");
    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/.se_config.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/hello.txt");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/shakespeare.txt");

    editor_buffer_set_cursor_point_virtual(buffer, 1, 0, 30);
    editor_buffer_set_cursor_point_to_end_of_line_virtual(buffer, 30);
    buf_print_fmt("vcursor: %i64, %i64\n",
                  editor_buffer_get_cursor_row_virtual(buffer, 0, 30),
                  editor_buffer_get_cursor_col_virtual(buffer, 0, 30));

    // print buffer
//    for (int64_t i = 0; i < editor_buffer_get_line_count(buffer); i++) {
//        int64_t length = editor_buffer_get_line_length(buffer, i);
//        buf_print_fmt("%str\n", editor_buffer_get_text_between_points(buffer, i, 0, i, length)->bytes);
//    }

    return 0;
}
