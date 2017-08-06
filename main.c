

#include "forward_types.h"
#include "editor_buffer.h"
#include "util.h"
#include "buf.h"

void
print_virtual_line(struct editor_buffer_t editor_buffer, int64_t line_number, int64_t virtual_line_length)
{
    struct buf_t *buf = editor_buffer_get_text_between_virtual_points(*editor_buffer.current_screen,
                                                                      line_number, 0,
                                                                      line_number + 1, 0,
                                                                      virtual_line_length);
    buf_print_fmt("%str\n", buf->bytes);
}

void
test_scratch(struct editor_buffer_t editor_buffer)
{
    int64_t virtual_line_length = 10;

    editor_buffer_open_file(editor_buffer, "/Users/chadrussell/Desktop/small_one_line.txt");
    editor_buffer_set_cursor_point_virtual(editor_buffer, 3, 0, virtual_line_length);

    SE_ASSERT(1);
}

int
main()
{
    struct editor_buffer_t editor_buffer = editor_buffer_create();
    test_scratch(editor_buffer);
    return 0;
}
