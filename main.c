
#include "forward_types.h"
#include "buf.h"
#include "editor_buffer.h"
#include "line_rope.h"

void print_buffer(struct editor_buffer_t buffer) {
    int64_t line_count = editor_buffer_get_line_count(buffer);
    for (int64_t i = 0; i < 100; i++) {
        struct buf_t *line_buf = editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0);
        buf_print_fmt("%str", line_buf->bytes);
    }
}

int
main()
{
    buf_id = 0;
    buf_size = 0;
    rope_id = 0;
    line_rope_id = 0;

//    struct editor_buffer_t buffer = editor_buffer_create(100);

//    editor_buffer_open_file(buffer, 100, "/Users/chadrussell/.se_config.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/hello.txt");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/very_large.txt");
//    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/Projects/text/shakespeare.txt");

    for (int i = 0; i < 100; i++) {
        struct editor_buffer_t buffer = editor_buffer_create(100);
        editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/menu.json");
        editor_buffer_destroy(buffer);
    }

//    buf_print_fmt("before: %i32\n", buffer.current_screen->lines->longest_child_line_length);

//    editor_buffer_set_cursor_point(buffer, 40, 118);
//    editor_buffer_delete(buffer);

//    buf_print_fmt("after: %i32", buffer.current_screen->lines->longest_child_line_length);

//    for (int i = 0; i < 100000; i++) {
//        *buffer.save_to_undo = 0;
//        editor_buffer_insert(buffer, "hello, world! :)");
//        editor_buffer_set_cursor_is_selection(buffer, 1);
//        editor_buffer_set_cursor_pos(buffer, 0);
//        editor_buffer_delete(buffer);
//        editor_buffer_undo(buffer, 0);
//    }

//    print_buffer(buffer);

//    editor_buffer_insert(buffer, "ooabbacc");
//    int64_t answer = editor_buffer_search_backward(buffer, "ba", 8);
//    buf_print_fmt("answer: %i64", answer);

//    editor_buffer_set_cursor_is_selection(buffer, 0);
//    editor_buffer_set_cursor_pos(buffer, 0);
//    editor_buffer_set_cursor_is_selection(buffer, 1);
//    editor_buffer_set_cursor_pos(buffer, editor_buffer_get_char_count(buffer));
//    editor_buffer_delete(buffer);
//    editor_buffer_set_cursor_is_selection(buffer, 0);
//    buf_print_fmt("line_count: %i64", editor_buffer_get_line_count_virtual(buffer, 80));

    return 0;
}
