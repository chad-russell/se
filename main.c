
#include "forward_types.h"
#include "line_rope.h"
#include "buf.h"
#include "editor_buffer.h"

int32_t
utf_number(const char *str)
{
    char first_byte = *str;

    if (first_byte >= '\x00' && first_byte <= '\x7F') {
        return (int32_t) first_byte;
    }
    if (first_byte >= '\xC2' && first_byte <= '\xDF') {
        int16_t first_two_bytes = *((int16_t *) str);
        return (int32_t) first_two_bytes;
    }
    if (first_byte >= '\xE0' && first_byte <= '\xEF') {
        int32_t first_four_bytes = *((int32_t *) str);
        return (first_four_bytes << 8);
    }
    if (first_byte >= '\xF0' && first_byte <= '\xF4') {
        int32_t first_four_bytes = *((int32_t *) str);
        return first_four_bytes;
    }
    SE_UNREACHABLE();
}

const char *
utf_offset(const char *str, int64_t offset)
{
    const char *offset_str = str;
    for (int64_t i = 0; i < offset; i++) {
        offset_str += bytes_in_codepoint_utf8(*offset_str);
    }
    return offset_str;
}

struct vector_t *
kmp_prefix(const char *str)
{
    int64_t str_len = unicode_strlen(str);

    struct vector_t *p = vector_init(str_len, sizeof(int64_t));
    p->length = str_len;

    int64_t i = 1;
    const char *str_i = str + bytes_in_codepoint_utf8(*str);

    int64_t j = 0;
    const char *str_j = str;

    while (i < str_len) {
        while (j > 0 && utf_number(str_j) != utf_number(str_i)) {
            j = *((int64_t *) vector_at(p, j - 1));
            str_j = utf_offset(str, j);
        }

        if (utf_number(str_j) == utf_number(str_i)) {
            j += 1;
            str_j += bytes_in_codepoint_utf8(*str_j);
        }

        vector_set_at(p, i, &j);

        i += 1;
        str_i += bytes_in_codepoint_utf8(*str_i);
    }

    return p;
}

int
main()
{
//    struct editor_buffer_t buffer = editor_buffer_create(30);

//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/very_large.txt");
//    editor_buffer_open_file(buffer, 30, "/Users/chadrussell/.se_config.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/menu.json");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/hello.txt");
//    editor_buffer_open_file(buffer, 80, "/Users/chadrussell/Desktop/shakespeare.txt");

//    editor_buffer_set_cursor_point_virtual(buffer, 1, 0, 30);
//    editor_buffer_set_cursor_point_to_end_of_line_virtual(buffer, 30);
//    buf_print_fmt("vcursor: %i64, %i64\n",
//                  editor_buffer_get_cursor_row_virtual(buffer, 0, 30),
//                  editor_buffer_get_cursor_col_virtual(buffer, 0, 30));

    // print buffer
//    for (int64_t i = 0; i < editor_buffer_get_line_count(buffer); i++) {
//        int64_t length = editor_buffer_get_line_length(buffer, i);
//        buf_print_fmt("%str\n", editor_buffer_get_text_between_points(buffer, i, 0, i, length)->bytes);
//    }

    buf_print_fmt("%vector_deref\n",
                  "%i64",
                  kmp_prefix("∂万∂万∂GT"),
                  ", ");

    return 0;
}
