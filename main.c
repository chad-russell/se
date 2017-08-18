
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

int
main()
{
    struct editor_buffer_t buffer = editor_buffer_create(80);

//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/very_large.txt");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/.se_config.json");
    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/hello.txt");

    for (uint32_t i = 0; i < 1000000; i++) {
        editor_buffer_insert(buffer, "a");
        editor_buffer_delete(buffer);
    }

//    editor_buffer_insert(buffer, "\n\n\n\n");
//
//    editor_buffer_add_cursor_at_point_virtual(buffer, 0, 0, 10);
//    editor_buffer_add_cursor_at_point_virtual(buffer, 1, 0, 10);
//    editor_buffer_add_cursor_at_point_virtual(buffer, 3, 0, 10);
//
//    editor_buffer_delete(buffer);
//
//    for (int64_t i = 0; i < buffer.current_screen->cursor_infos->length; i++) {
//        int64_t row = editor_buffer_get_cursor_row(buffer, i);
//        int64_t col = editor_buffer_get_cursor_col(buffer, i);
//        buf_print_fmt("cursor #%i64 is at (%i64, %i64)\n", i, row, col);
//    }

    // print buffer
//    for (int64_t i = 0; i < editor_buffer_get_line_count(buffer); i++) {
//        buf_print_fmt("%str", editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0)->bytes);
//    }

    return 0;
}
