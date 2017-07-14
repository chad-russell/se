#include <stdio.h>
#include <stdlib.h>

#include "forward_types.h"
#include "rope.h"
#include "util.h"
#include "circular_buffer.h"
#include "editor_buffer.h"

void
test_scratch(struct editor_buffer_t editor_buffer)
{
    editor_buffer_open_file(editor_buffer, "/Users/chadrussell/Desktop/big_one_line.txt");

    const char *line1_to_line2 = editor_buffer_get_text_between_points(editor_buffer, 0, 0, 0, 10);

    SE_ASSERT(1);
}

int
main()
{
    struct editor_buffer_t editor_buffer;
    editor_buffer.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);
    editor_buffer.global_undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), GLOBAL_UNDO_BUFFER_SIZE);

    struct editor_screen_t screen;
    screen.rn = rope_leaf_init("");
    screen.cursor_info.cursor_line = 0;
    screen.cursor_info.cursor_col = 0;
    undo_stack_append(editor_buffer, screen);

    test_scratch(editor_buffer);

    return 0;
}
