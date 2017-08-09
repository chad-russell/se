

#include "forward_types.h"
#include "editor_buffer.h"
#include "util.h"
#include "buf.h"

void
test_scratch(struct editor_buffer_t editor_buffer)
{
    for (int32_t i = 0; i < 10; i++) {
        editor_buffer_insert(editor_buffer, "a");
    }

    buf_print_fmt("line 0 length: %i64", editor_buffer_get_line_length(editor_buffer, 0));

    SE_ASSERT(1);
}

int
main()
{
    struct editor_buffer_t editor_buffer = editor_buffer_create();
    test_scratch(editor_buffer);
    return 0;
}
