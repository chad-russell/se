#include <stdlib.h>

#include "forward_types.h"
#include "rope.h"
#include "util.h"
#include "editor_buffer.h"

void
test_scratch(struct editor_buffer_t editor_buffer)
{
    editor_buffer_open_file(editor_buffer, "/Users/chadrussell/Projects/text/hello.txt");
    const char *contents_1 = editor_buffer_get_text_between_points(editor_buffer, 0, 0, 1, 0);
    const char *contents_2 = editor_buffer_get_text_between_points(editor_buffer, 1, 0, 2, 0);
    const char *contents_3 = editor_buffer_get_text_between_points(editor_buffer, 2, 0, 3, 0);
    const char *contents_4 = editor_buffer_get_text_between_points(editor_buffer, 3, 0, 4, 0);

    SE_ASSERT(1);
}

int
main()
{
    struct editor_buffer_t editor_buffer = editor_buffer_create();

    struct editor_screen_t screen;
    screen.rn = rope_leaf_init("");
    screen.cursor_info.char_pos = 0;
    undo_stack_append(editor_buffer, screen);

    test_scratch(editor_buffer);

    return 0;
}
