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

    editor_buffer.undo_idx = se_alloc(1, sizeof(int64_t));
    *editor_buffer.undo_idx = 0;

    editor_buffer.global_undo_idx = se_alloc(1, sizeof(int64_t));
    *editor_buffer.global_undo_idx = 0;

    editor_buffer.current_screen = se_alloc(1, sizeof(struct editor_screen_t));

    struct editor_screen_t screen;

    screen.cursor_info.char_pos = 0;
    screen.cursor_info.row = 0;
    screen.cursor_info.col = 0;
    screen.rn = rope_leaf_init("");

    screen.line_lengths = vector_init(32, sizeof(int64_t));

    int64_t zero = 0;
    vector_append(screen.line_lengths, &zero);

    undo_stack_append(editor_buffer, screen);

    return editor_buffer;
}

struct editor_screen_t
editor_buffer_get_current_screen(struct editor_buffer_t editor_buffer)
{
    return *editor_buffer.current_screen;
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

    free(editor_buffer.undo_idx);
    free(editor_buffer.global_undo_idx);
    free(editor_buffer.current_screen);
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

int64_t
editor_screen_calculate_line_length(struct editor_screen_t screen, int64_t line)
{
    int64_t char_number_at_current_line = rope_char_number_at_line(screen.rn, line);
    int64_t char_number_at_next_line = rope_char_number_at_line(screen.rn, line + 1);
    return char_number_at_next_line - char_number_at_current_line - 1;
}

struct editor_screen_t
editor_buffer_open_file(struct editor_buffer_t editor_buffer, const char *file_path)
{
    struct editor_screen_t screen;

    screen.cursor_info.char_pos = 0;
    screen.cursor_info.row = 0;
    screen.cursor_info.col = 0;

    screen.rn = read_file(file_path);
    screen.line_lengths = vector_init(32, sizeof(int64_t));

    // todo(chad):
    // re-calculate this on every edit
    // hopefully in a smart way, so that we don't have to calculate the whole file every time
    int64_t line_count = rope_total_line_break_length(screen.rn) + 1;

    int64_t zero = 0;
    while (screen.line_lengths->length < line_count) {
        vector_append(screen.line_lengths, &zero);
    }

    for (int64_t line_num = 0; line_num < line_count; line_num++) {
        int64_t line_length_i = editor_screen_calculate_line_length(screen, line_num);
        vector_set_at(screen.line_lengths, line_num, &line_length_i);
    }

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

    struct editor_screen_t screen = editor_buffer_get_current_screen(editor_buffer);
    int32_t err = fprintf(file, "%s", editor_buffer_get_all_text(screen));

    fclose(file);

    return err;
}

struct editor_screen_t
editor_buffer_insert(struct editor_buffer_t editor_buffer, const char *text)
{
    struct rope_t *edited = rope_insert(editor_buffer.current_screen->rn, editor_buffer.current_screen->cursor_info.char_pos, text);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;

    // position cursor at the end of the insertion
    edited_screen.cursor_info = editor_buffer.current_screen->cursor_info;
    edited_screen.cursor_info.char_pos += 1;
    editor_screen_set_line_and_col_for_char_pos(&edited_screen);

    // todo(chad): need to copy this!
    edited_screen.line_lengths = editor_buffer.current_screen->line_lengths;

    undo_stack_append(editor_buffer, edited_screen);

    return edited_screen;
}

struct editor_screen_t
editor_buffer_delete(struct editor_buffer_t editor_buffer)
{
    if (editor_buffer.current_screen->cursor_info.char_pos == 0) { return *editor_buffer.current_screen; }

    struct rope_t *edited = rope_delete(editor_buffer.current_screen->rn,
                                        editor_buffer.current_screen->cursor_info.char_pos - 1,
                                        editor_buffer.current_screen->cursor_info.char_pos);

    struct editor_screen_t edited_screen;
    edited_screen.rn = edited;

    edited_screen.cursor_info = editor_buffer.current_screen->cursor_info;
    edited_screen.cursor_info.char_pos -= 1;
    editor_screen_set_line_and_col_for_char_pos(&edited_screen);

    // todo(chad): need to copy this!
    edited_screen.line_lengths = editor_buffer.current_screen->line_lengths;

    undo_stack_append(editor_buffer, edited_screen);

    return edited_screen;
}

struct editor_screen_t
editor_buffer_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.undo_buffer, undo_idx);
    global_only_undo_stack_append(editor_buffer, *undone_screen);

    *editor_buffer.undo_idx = undo_idx;

    return *undone_screen;
}

struct editor_screen_t
editor_buffer_global_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.global_undo_buffer, undo_idx);

    *editor_buffer.global_undo_idx = undo_idx;
    *editor_buffer.undo_idx = editor_buffer.undo_buffer->length;
    *editor_buffer.current_screen = *undone_screen;

    return *undone_screen;
}

int64_t
editor_screen_get_line_count(struct editor_screen_t editor_screen)
{
    return 1 + rope_total_line_break_length(editor_screen.rn);
}

int64_t
editor_screen_get_line_length(struct editor_screen_t editor_screen, int64_t line)
{
    int64_t *line_length = vector_at(editor_screen.line_lengths, line);
    return *line_length;
}

int64_t
editor_buffer_get_virtual_line_count(struct editor_buffer_t editor_buffer, int64_t virtual_line_length)
{
    int64_t virtual_line_count = 0;
    int64_t lines = editor_screen_get_line_count(*editor_buffer.current_screen);
    for (int64_t line = 0; line < lines; line++) {
        int64_t line_length = editor_screen_get_line_length(*editor_buffer.current_screen, line);

        if (line_length == 0) {
            virtual_line_count += 1;
        } else {
            virtual_line_count += line_length / virtual_line_length;

            int64_t remainder = line_length % virtual_line_length;
            if (remainder != 0) {
                virtual_line_count += 1;
            }
        }
    }

    return virtual_line_count;
}

int64_t
editor_screen_get_char_count(struct editor_screen_t editor_screen)
{
    return rope_total_char_length(editor_screen.rn);
}

int64_t
editor_buffer_get_char_number_at_line(struct editor_buffer_t editor_buffer, int64_t i)
{
    struct editor_screen_t screen = editor_buffer_get_current_screen(editor_buffer);
    return rope_char_number_at_line(screen.rn, i);
}

struct buf_t *
editor_buffer_get_text_between_characters(struct editor_screen_t editor_screen, int64_t start, int64_t end)
{
    struct rope_t *r = editor_screen.rn;

    struct buf_t *buf = buf_init(end - start + 1);
    for (int64_t i = start; i < end; i++) {
        const char *char_at = rope_char_at(r, i);

        // char_at will only be null if we are looking for a character past the end of the rope
        if (char_at != NULL) {
            buf_write_bytes(buf, char_at, bytes_in_codepoint_utf8(*char_at));
        }
    }

    return buf;
}

struct buf_t *
editor_buffer_get_text_between_points(struct editor_screen_t editor_screen,
                                      int64_t start_line,
                                      int64_t start_col,
                                      int64_t end_line,
                                      int64_t end_col)
{
    struct rope_t *r = editor_screen.rn;

    int64_t start_line_char_number = rope_char_number_at_line(r, start_line);
    if (start_line_char_number < 0) { start_line_char_number = 0; }

    int64_t end_line_char_number;
    int64_t total_lines = 1 + rope_total_line_break_length(editor_screen.rn);
    if (end_line >= total_lines) {
        end_line_char_number = rope_total_char_length(r);
    } else {
        end_line_char_number = rope_char_number_at_line(r, end_line);
    }

    return editor_buffer_get_text_between_characters(editor_screen,
                                                     start_line_char_number + start_col,
                                                     end_line_char_number + end_col);
}

int64_t
editor_screen_character_position_for_virtual_point(struct editor_screen_t editor_screen,
                                                   int64_t line,
                                                   int64_t col,
                                                   int64_t virtual_line_length)
{
    int64_t current_line = 0;
    int64_t current_line_length = editor_screen_get_line_length(editor_screen, current_line);

    int64_t virtual_newlines_including_current_line = current_line_length / virtual_line_length;
    int64_t remainder = current_line_length % virtual_line_length;
    if (remainder != 0) {
        virtual_newlines_including_current_line += 1;
    }

    int64_t virtual_newlines_before_current_line = 0;
    int64_t total_lines = editor_screen_get_line_count(editor_screen);

    while (virtual_newlines_including_current_line - 1 < line && current_line + 1 < total_lines) {
        virtual_newlines_before_current_line = virtual_newlines_including_current_line;

        current_line += 1;

        current_line_length = editor_screen_get_line_length(editor_screen, current_line);
        virtual_newlines_including_current_line += current_line_length / virtual_line_length;
        remainder = current_line_length % virtual_line_length;
        if (remainder != 0) {
            virtual_newlines_including_current_line += 1;
        }
    }

    int64_t char_pos = rope_char_number_at_line(editor_screen.rn, current_line);
    int64_t additional_virtual_newlines_needed = line - virtual_newlines_before_current_line;
    int64_t additional_cols_needed = additional_virtual_newlines_needed * virtual_line_length;

    int64_t final_char_pos = char_pos + additional_cols_needed + col;

    int64_t char_pos_at_start_of_current_line = rope_char_number_at_line(editor_screen.rn, current_line);

    // make sure it doesn't go past the end of the current line
    int64_t end_of_current_line = char_pos_at_start_of_current_line + current_line_length;
    if (final_char_pos > end_of_current_line) {
        final_char_pos = end_of_current_line;
    }

    return final_char_pos;
}

struct buf_t *
editor_buffer_get_text_between_virtual_points(struct editor_screen_t editor_screen,
                                              int64_t start_line, int64_t start_col,
                                              int64_t end_line, int64_t end_col,
                                              int64_t virtual_line_length)
{
    int64_t computed_start_char = editor_screen_character_position_for_virtual_point(editor_screen, start_line, start_col, virtual_line_length);
    int64_t computed_end_char = editor_screen_character_position_for_virtual_point(editor_screen, end_line, end_col, virtual_line_length);

    const char *computed_start = rope_char_at(editor_screen.rn, computed_start_char);
    if (computed_start != NULL && *computed_start == '\n') {
        computed_start_char += 1;
    }

    const char *one_before_computed_end = rope_char_at(editor_screen.rn, computed_end_char - 1);
    if (one_before_computed_end != NULL && *one_before_computed_end == '\n') {
        computed_end_char -= 1;
    }

    return editor_buffer_get_text_between_characters(editor_screen, computed_start_char, computed_end_char);
}

const char *
editor_buffer_get_all_text(struct editor_screen_t editor_screen)
{
    // todo(chad): this leaks the buf :(
    return buf_init_fmt("%rope", editor_screen.rn)->bytes;
}

int64_t
editor_screen_get_cursor_pos(struct editor_screen_t editor_screen)
{
    return editor_screen.cursor_info.char_pos;
}

int64_t
editor_screen_get_cursor_row(struct editor_screen_t editor_screen)
{
    return editor_screen.cursor_info.row;
}

int64_t
editor_screen_get_cursor_col(struct editor_screen_t editor_screen)
{
    return editor_screen.cursor_info.col;
}

int64_t
editor_screen_get_cursor_row_virtual(struct editor_screen_t editor_screen, int64_t virtual_line_length)
{
    int64_t current_line_length = editor_screen_get_line_length(editor_screen, 0);

    int64_t virtual_newlines_including_current_line = current_line_length / virtual_line_length;
    int64_t remainder = current_line_length % virtual_line_length;
    if (remainder != 0) {
        virtual_newlines_including_current_line += 1;
    }

    int64_t virtual_newlines_before_current_line = 0;

    for (int64_t current_line = 0; current_line < editor_screen.cursor_info.row; current_line++) {
        virtual_newlines_before_current_line = virtual_newlines_including_current_line;

        current_line_length = editor_screen_get_line_length(editor_screen, current_line);
        virtual_newlines_including_current_line += current_line_length / virtual_line_length;
        remainder = current_line_length % virtual_line_length;
        if (remainder != 0) {
            virtual_newlines_including_current_line += 1;
        }
    }

    int64_t extra_from_cols = editor_screen.cursor_info.col / virtual_line_length;
    return virtual_newlines_before_current_line + extra_from_cols;
}

int64_t
editor_screen_get_cursor_col_virtual(struct editor_screen_t editor_screen, int64_t virtual_line_length)
{
    return editor_screen.cursor_info.col % virtual_line_length;
}

struct editor_screen_t
editor_buffer_set_cursor_point_virtual(struct editor_buffer_t editor_buffer, int64_t row, int64_t col, int64_t virtual_line_length)
{
    struct editor_screen_t *screen = editor_buffer.current_screen;

    int64_t char_pos = editor_screen_character_position_for_virtual_point(*screen, row, col, virtual_line_length);
    int64_t total_length = rope_total_char_length(screen->rn);
    if (char_pos > total_length) {
        char_pos = total_length;
    }
    return editor_buffer_set_cursor_pos(editor_buffer, char_pos);
}

int64_t
editor_buffer_get_undo_size(struct editor_buffer_t editor_buffer)
{
    return editor_buffer.undo_buffer->length;
}

int64_t
editor_buffer_get_global_undo_size(struct editor_buffer_t editor_buffer)
{
    return editor_buffer.global_undo_buffer->length;
}

int64_t
editor_buffer_get_undo_index(struct editor_buffer_t editor_buffer)
{
    return *editor_buffer.undo_idx;
}

int64_t
editor_buffer_get_global_undo_index(struct editor_buffer_t editor_buffer)
{
    return *editor_buffer.global_undo_idx;
}

struct editor_screen_t
editor_buffer_set_cursor_pos(struct editor_buffer_t editor_buffer, int64_t cursor)
{
    struct editor_screen_t *screen = editor_buffer.current_screen;

    screen->cursor_info.char_pos = cursor;

    if (screen->cursor_info.char_pos < 0) {
        screen->cursor_info.char_pos = 0;
    }

    int64_t total_length = rope_total_char_length(screen->rn);
    if (screen->cursor_info.char_pos > total_length) {
        screen->cursor_info.char_pos = total_length;
    }

    editor_screen_set_line_and_col_for_char_pos(screen);

    return *screen;
}

int64_t
editor_buffer_get_end_of_row(struct editor_buffer_t editor_buffer, int64_t row)
{
    struct editor_screen_t screen = editor_buffer_get_current_screen(editor_buffer);

    int64_t num_lines = editor_screen_get_line_count(screen);
    if (row + 1 >= num_lines) {
        return editor_screen_get_char_count(screen);
    }

    int64_t start_of_next_line = editor_buffer_get_char_number_at_line(editor_buffer, row + 1);
    return start_of_next_line - 1;
}

struct editor_screen_t
editor_buffer_set_cursor_point(struct editor_buffer_t editor_buffer, int64_t row, int64_t col)
{
    struct editor_screen_t *screen = editor_buffer.current_screen;

    int64_t computed_row = row;
    int64_t row_count = editor_screen_get_line_count(*screen);
    if (computed_row > row_count - 1) { computed_row = row_count - 1; }
    if (computed_row < 0) { computed_row = 0; }

    int64_t start_for_row = editor_buffer_get_char_number_at_line(editor_buffer, computed_row);
    int64_t end_of_row = editor_buffer_get_end_of_row(editor_buffer, computed_row);
    int64_t row_length = end_of_row - start_for_row;

    int64_t computed_col = row_length < col ? row_length : col;

    screen->cursor_info.char_pos = start_for_row + computed_col;
    screen->cursor_info.row = computed_row;
    screen->cursor_info.col = computed_col;

    return *screen;
}

struct editor_screen_t
editor_buffer_set_cursor_pos_relative(struct editor_buffer_t editor_buffer, int64_t relative_cursor)
{
    struct editor_screen_t *screen = editor_buffer.current_screen;

    screen->cursor_info.char_pos += relative_cursor;

    if (screen->cursor_info.char_pos < 0) {
        screen->cursor_info.char_pos = 0;
    }

    int64_t total_length = rope_total_char_length(screen->rn);
    if (screen->cursor_info.char_pos > total_length) {
        screen->cursor_info.char_pos = total_length;
    }

    editor_screen_set_line_and_col_for_char_pos(screen);

    return *screen;
}

const char *
editor_buffer_get_buf_bytes(struct buf_t *buf)
{
    return buf->bytes;
}

void
editor_buffer_free_buf(struct buf_t *buf)
{
    buf_free(buf);
}

void
editor_screen_set_line_and_col_for_char_pos(struct editor_screen_t *screen)
{
    if (screen == NULL) { return; }

    int64_t row = rope_get_line_number_for_char_pos(screen->rn, screen->cursor_info.char_pos);
    screen->cursor_info.row = row;

    int64_t row_start = rope_char_number_at_line(screen->rn, row);
    int64_t col = screen->cursor_info.char_pos - row_start;
    screen->cursor_info.col = col;
}
