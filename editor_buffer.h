#ifndef SE_EDITOR_BUFFER_H
#define SE_EDITOR_BUFFER_H

#include "forward_types.h"

struct editor_buffer_t
editor_buffer_create();

void
editor_buffer_destroy(struct editor_buffer_t editor_buffer);

struct editor_screen_t
editor_buffer_open_file(struct editor_buffer_t editor_buffer, const char *file_path);

int32_t
editor_buffer_save_file(struct editor_buffer_t editor_buffer);

int32_t
editor_save_file(struct editor_buffer_t editor_buffer);

struct editor_screen_t
editor_buffer_insert(struct editor_buffer_t editor_buffer, const char *text);

struct editor_screen_t
editor_buffer_delete(struct editor_buffer_t editor_buffer);

struct editor_screen_t
editor_buffer_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx);

struct editor_screen_t
editor_buffer_global_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx);

int64_t
editor_buffer_get_line_count(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_char_number_at_line(struct editor_buffer_t editor_buffer, int64_t i);

const char *
editor_buffer_get_text_between_characters(struct editor_buffer_t editor_buffer, int64_t start, int64_t end);

const char *
editor_buffer_get_text_between_points(struct editor_buffer_t editor_buffer, int64_t start_line, int64_t start_col, int64_t end_line, int64_t end_col);

const char *
editor_buffer_get_all_text(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_cursor_pos(struct editor_buffer_t editor_buffer);

void
editor_buffer_set_cursor_pos_relative(struct editor_buffer_t editor_buffer, int64_t relative_cursor);

#endif //SE_EDITOR_BUFFER_H
