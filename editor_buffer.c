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
    editor_buffer.file_path = buf_init(0);

    struct editor_screen_t screen;
    screen.rn = rope_leaf_init("");

    screen.cursor_info.char_pos = 0;

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
    screen.cursor_info.char_pos = 0;
    screen.rn = read_file(file_path);

    undo_stack_append(editor_buffer, screen);

    *editor_buffer.file_path = *buf_init_fmt("%str", file_path);

    return screen;
}

int32_t
editor_buffer_save_file(struct editor_buffer_t editor_buffer)
{
    if (editor_buffer.file_path == NULL) { return -1; }

    FILE *file = fopen(editor_buffer.file_path->bytes, "w");
    if (file == NULL) {
        fprintf(stderr, "error opening file\n");
        exit(-1);
    }

    rewind(file);

    int32_t err = fprintf(file, "%s", editor_buffer_get_all_text(editor_buffer));

    fclose(file);

    return err;
}

struct editor_screen_t
editor_buffer_insert(struct editor_buffer_t editor_buffer, const char *text)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    struct rope_t *edited = rope_insert(screen->rn, screen->cursor_info.char_pos, text);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;

    // position cursor at the end of the insertion
    edited_screen.cursor_info = screen->cursor_info;
    edited_screen.cursor_info.char_pos += 1;

    undo_stack_append(editor_buffer, edited_screen);

    return edited_screen;
}

struct editor_screen_t
editor_buffer_delete(struct editor_buffer_t editor_buffer)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    if (screen->cursor_info.char_pos == 0) { return *screen; }

    struct rope_t *edited = rope_delete(screen->rn, screen->cursor_info.char_pos - 1, screen->cursor_info.char_pos);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;

    edited_screen.cursor_info = screen->cursor_info;
    if (edited_screen.cursor_info.char_pos > 0) {
        edited_screen.cursor_info.char_pos -= 1;
    }

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

int64_t
editor_buffer_get_line_count(struct editor_buffer_t editor_buffer)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    return rope_total_line_break_length(screen->rn);
}

int64_t
editor_buffer_get_char_number_at_line(struct editor_buffer_t editor_buffer, int64_t i)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    return rope_char_number_at_line(screen->rn, i);
}

const char *
editor_buffer_get_text_between_characters(struct editor_buffer_t editor_buffer, int64_t start, int64_t end)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);
    struct rope_t *r = screen->rn;

    struct buf_t *buf = buf_init(end - start + 1);
    for (int64_t i = start; i < end; i++) {
        const char *char_at = rope_char_at(r, i);

        // char_at will only be null if we are looking for a character past the end of the rope
        if (char_at != NULL) {
            buf_write_bytes(buf, char_at, bytes_in_codepoint_utf8(*char_at));
        }
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

int64_t
editor_buffer_get_cursor_pos(struct editor_buffer_t editor_buffer)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    return screen->cursor_info.char_pos;
}

void
editor_buffer_set_cursor_pos_relative(struct editor_buffer_t editor_buffer, int64_t relative_cursor)
{
    struct editor_screen_t *screen = circular_buffer_at_end(editor_buffer.undo_buffer);

    screen->cursor_info.char_pos += relative_cursor;

    if (screen->cursor_info.char_pos < 0) {
        screen->cursor_info.char_pos = 0;
    }

    int64_t total_length = rope_total_char_length(screen->rn);
    if (screen->cursor_info.char_pos > total_length) {
        screen->cursor_info.char_pos = total_length;
    }
}
