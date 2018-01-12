
#include "forward_types.h"
#include "buf.h"
#include "editor_buffer.h"
#include "line_rope.h"

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
    struct editor_buffer_t buffer = editor_buffer_create(100);

//    editor_buffer_open_file(buffer, 100, "/Users/chadrussell/.se_config.json");
    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/hello.txt");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Projects/text/very_large.txt");
//    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/Projects/text/shakespeare.txt");

    editor_buffer_set_cursor_point(buffer, 284468, 32);

    // cursor_pos IS correct 100%
    int64_t cursor_pos = editor_buffer_get_cursor_pos(buffer, 0);

    // should be 37 chars after where we are now (i.e. cursor_pos + 37, or 14258761)
    int64_t found = editor_buffer_search_forward(buffer, "G", cursor_pos);

    // cursor_pos: 14258724
    // start_char: 14258724
    // byte_offset: 14259745

//    buf_print_fmt("char for byte at 14258724: %i64\n", rope_char_for_byte_at(buffer.current_screen->text, 14258724));
//    buf_print_fmt("char for byte at 14259745: %i64\n", rope_char_for_byte_at(buffer.current_screen->text, 14259745));

    return 0;
}
