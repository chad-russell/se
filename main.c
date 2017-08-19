
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
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/hello.txt");
    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/shakespeare.txt");

    editor_buffer_set_cursor_point(buffer, 1, 0);
    editor_buffer_insert(buffer, "\n");

    // print buffer
//    for (int64_t i = 0; i < editor_buffer_get_line_count(buffer); i++) {
    for (int64_t i = 0; i < 4; i++) {
        int64_t length = editor_buffer_get_line_length(buffer, i);
//        buf_print_fmt("%str\n", editor_buffer_get_text_between_points(buffer, i, 0, i, length)->bytes);

//        buf_print_fmt("%str", editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0)->bytes);
        buf_print_fmt("%i64: %i64\n", i, length);
    }

    return 0;
}
