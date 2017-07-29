

#include "forward_types.h"
#include "editor_buffer.h"
#include "util.h"

void
test_scratch(struct editor_buffer_t editor_buffer)
{
    editor_buffer_open_file(editor_buffer, "/Users/chadrussell/Desktop/hello.txt");
    editor_buffer_set_cursor_point(editor_buffer, 16, 7);

    int64_t line0 = editor_buffer_get_char_number_at_line(editor_buffer, 0);
    int64_t line1 = editor_buffer_get_char_number_at_line(editor_buffer, 1);
    int64_t line_last = editor_buffer_get_char_number_at_line(editor_buffer, 15);
    int64_t line_last_end = editor_buffer_get_end_of_row(editor_buffer, 15);

    SE_ASSERT(1);
}

int
main()
{
    struct editor_buffer_t editor_buffer = editor_buffer_create();
    test_scratch(editor_buffer);
    return 0;
}
