
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

int
main()
{
    struct editor_buffer_t buffer = editor_buffer_create();
    editor_buffer_open_file(buffer, "/Users/chadrussell/Desktop/shakespeare.txt");

    for (int i = 0; i < 100; i++) {
        struct buf_t *lines = editor_buffer_get_text_between_points_virtual(buffer, i, 0, i + 1, 0, 10);
//        struct buf_t *lines = editor_buffer_get_text_between_points(buffer, i, 0, i + 1, 0);
        buf_print_fmt("%str\n", lines->bytes);
        buf_free(lines);
    }

    return 0;
}
