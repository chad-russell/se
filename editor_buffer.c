#include <stdio.h>
#include "editor_buffer.h"
#include "rope.h"
#include "circular_buffer.h"
#include "buf.h"

struct editor_buffer_t
editor_buffer_create()
{
    struct editor_buffer_t editor_buffer;
    editor_buffer.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);
    editor_buffer.global_undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), GLOBAL_UNDO_BUFFER_SIZE);

    struct editor_screen_t screen;
    screen.rn = rope_leaf_init("");
    screen.cursor_info.cursor_line = 0;
    screen.cursor_info.cursor_col = 0;
    undo_stack_append(editor_buffer, screen);

    return editor_buffer;
}

void
editor_buffer_destroy(struct editor_buffer_t editor_buffer)
{
    for (int64_t i = 0; i < editor_buffer.undo_buffer->length; i++) {
        struct editor_screen_t *screen = circular_buffer_at(editor_buffer.undo_buffer, i);
        rope_dec_rc(screen->rn);
    }

    for (int64_t i = 0; i < editor_buffer.global_undo_buffer->length; i++) {
        struct editor_screen_t *screen = circular_buffer_at(editor_buffer.undo_buffer, i);
        rope_dec_rc(screen->rn);
    }
}

struct rope_t *
read_file(const char *file_path)
{
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "error opening file\n");
        exit(-1);
    }

    int err;

    err = fseek(file, 0, SEEK_END);
    if (err == -1) {
        fprintf(stderr, "fseek failed\n");
    }

    int64_t file_size = ftell(file);
    if (file_size == -1) {
        fprintf(stderr, "error determining size of file\n");
    }

    rewind(file);

    char *src = (char *) se_alloc(file_size + 1, sizeof(char));
    if (src == NULL) {
        fprintf(stderr, "could not allocate for reading file into memory\n");
    }

    size_t bytes_read = fread(src, sizeof(char), (size_t) file_size, file);
    if (bytes_read != file_size) {
        fprintf(stderr, "error reading file\n");
        exit(-1);
    }

    src[file_size] = '\0';

    err = fclose(file);
    if (err != 0) {
        fprintf(stderr, "warning: failure closing file\n");
    }

    return rope_leaf_init(src);
}

struct editor_screen_t
editor_buffer_open_file(struct editor_buffer_t editor_buffer, const char *file_path)
{
    struct editor_screen_t screen;
    screen.rn = read_file(file_path);
    screen.cursor_info.cursor_line = 0;
    screen.cursor_info.cursor_col = 0;
    undo_stack_append(editor_buffer, screen);

    return screen;
}

struct editor_screen_t
editor_buffer_insert(struct editor_buffer_t editor_buffer, int64_t i, const char *text)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    struct rope_t *edited = rope_insert(screen->rn, i, text);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;
    edited_screen.cursor_info = screen->cursor_info;
    undo_stack_append(editor_buffer, edited_screen);

    return edited_screen;
}

struct editor_screen_t
editor_buffer_append(struct editor_buffer_t editor_buffer, const char *text)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    int64_t end = rope_total_char_length(screen->rn);
    return editor_buffer_insert(editor_buffer, end, text);
}

struct editor_screen_t
editor_buffer_delete(struct editor_buffer_t editor_buffer, int64_t start, int64_t end)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    struct rope_t *edited = rope_delete(screen->rn, start, end);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;
    edited_screen.cursor_info = screen->cursor_info;
    undo_stack_append(editor_buffer, edited_screen);

    return edited_screen;
}

struct editor_screen_t
editor_buffer_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.undo_buffer, undo_idx);
    global_only_undo_stack_append(editor_buffer, *undone_screen);
    return *undone_screen;
}

struct editor_screen_t
editor_buffer_global_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.global_undo_buffer, undo_idx);
    return *undone_screen;
}

const char *
editor_buffer_get_text_between_characters(struct editor_buffer_t editor_buffer, int64_t start, int64_t end)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    struct rope_t *r = screen->rn;

    struct buf_t *buf = buf_init(end - start + 1);
    for (int64_t i = start; i < end; i++) {
        const char *char_at = rope_char_at(r, i);
        buf_write_bytes(buf, char_at, bytes_in_codepoint_utf8(*char_at));
    }

    // todo(chad): this leaks :(. Need to figure out what to do with buf
    return buf->bytes;
}

const char *
editor_buffer_get_text_between_points(struct editor_buffer_t editor_buffer, int64_t start_line, int64_t start_col, int64_t end_line, int64_t end_col)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    struct rope_t *r = screen->rn;

    int64_t start_line_char_number = rope_char_number_at_line(r, start_line);
    int64_t end_line_char_number = rope_char_number_at_line(r, end_line);

    return editor_buffer_get_text_between_characters(editor_buffer, start_line_char_number + start_col, end_line_char_number + end_col);
}

const char *
editor_buffer_get_all_text(struct editor_buffer_t editor_buffer)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    // todo(chad): this leaks the buf :(
    return buf_init_fmt("%rope", screen->rn)->bytes;
}
