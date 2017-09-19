
#include <string.h>
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

void print_buffer(struct editor_buffer_t buffer) {
    int64_t line_count = editor_buffer_get_line_count(buffer);
    for (int64_t i = 0; i < 1; i++) {
        struct buf_t *line_buf = editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0);
        buf_print_fmt("%str\n", line_buf->bytes);
    }
}

int
main()
{
    struct editor_buffer_t buffer = editor_buffer_create(30);

//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/very_large.txt");
//    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/.se_config.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/hello.txt");
//    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/Desktop/shakespeare.txt");

//    for (int i = 0; i < 100000; i++) {
//        *buffer.save_to_undo = 0;
//        editor_buffer_insert(buffer, "hello, world! :)");
//        editor_buffer_set_cursor_is_selection(buffer, 1);
//        editor_buffer_set_cursor_pos(buffer, 0);
//        editor_buffer_delete(buffer);
//        editor_buffer_undo(buffer, 0);
//    }

//    print_buffer(buffer);

    editor_buffer_insert(buffer, "ooabbacc");
    int64_t answer = editor_buffer_search_backward(buffer, "ba", 8);
    buf_print_fmt("answer: %i64", answer);

    return 0;
}
