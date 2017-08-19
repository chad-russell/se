#ifndef SE_EDITOR_BUFFER_H
#define SE_EDITOR_BUFFER_H

#include "forward_types.h"

struct editor_buffer_t
editor_buffer_create(int64_t virtual_line_length);

void
editor_buffer_destroy(struct editor_buffer_t editor_buffer);

void
editor_buffer_open_file(struct editor_buffer_t editor_buffer, int64_t virtual_line_length, const char *file_path);

int32_t
editor_buffer_save_file(struct editor_buffer_t editor_buffer);

int32_t
editor_buffer_save_file_as(struct editor_buffer_t editor_buffer, const char *file_name);

int8_t
editor_buffer_has_file_path(struct editor_buffer_t editor_buffer);

void
editor_buffer_insert(struct editor_buffer_t editor_buffer, const char *text);

void
editor_buffer_delete(struct editor_buffer_t editor_buffer);

void
editor_buffer_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx);

void
editor_buffer_global_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx);

int64_t
editor_buffer_get_line_length(struct editor_buffer_t editor_buffer, int64_t line);

int64_t
editor_buffer_get_line_length_virtual(struct editor_buffer_t editor_buffer, int64_t line, int64_t virtual_line_length);

int64_t
editor_buffer_get_line_count(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_line_count_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length);

int64_t
editor_buffer_get_char_count(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_char_number_at_line(struct editor_buffer_t editor_buffer, int64_t i);

struct buf_t *
editor_buffer_get_text_between_characters(struct editor_buffer_t editor_buffer, int64_t start, int64_t end);

struct buf_t *
editor_buffer_get_text_between_points(struct editor_buffer_t editor_buffer, int64_t start_line, int64_t start_col, int64_t end_line, int64_t end_col);

struct buf_t *
editor_buffer_get_text_between_points_virtual(struct editor_buffer_t editor_buffer, int64_t start_line,
                                              int64_t start_col, int64_t end_line, int64_t end_col,
                                             int64_t virtual_line_length);

int64_t
editor_buffer_get_cursor_pos(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

int64_t
editor_buffer_get_cursor_row(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

int64_t
editor_buffer_get_cursor_col(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

int64_t
editor_buffer_get_cursor_row_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length);

int64_t
editor_buffer_get_cursor_col_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length);

void
editor_buffer_set_cursor_point_virtual(struct editor_buffer_t editor_buffer, int64_t row, int64_t col, int64_t virtual_line_length);

void
editor_buffer_set_cursor_point_virtual_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t row, int64_t col, int64_t virtual_line_length);

void
sort_and_merge_cursors(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_undo_size(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_global_undo_size(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_undo_index(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_global_undo_index(struct editor_buffer_t editor_buffer);

int64_t
editor_buffer_get_cursor_count(struct editor_buffer_t editor_buffer);

void
editor_buffer_set_cursor_pos(struct editor_buffer_t editor_buffer, int64_t cursor);

void
editor_buffer_set_cursor_pos_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t cursor);

int64_t
editor_buffer_get_end_of_row(struct editor_buffer_t editor_buffer, int64_t row);

void
editor_buffer_set_cursor_point_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_index, int64_t row, int64_t col);

void
editor_buffer_set_cursor_point(struct editor_buffer_t editor_buffer, int64_t row, int64_t col);

void
editor_buffer_set_cursor_point_to_start_of_line(struct editor_buffer_t editor_buffer);

void
editor_buffer_set_cursor_point_to_end_of_line(struct editor_buffer_t editor_buffer);

void
editor_buffer_set_cursor_point_to_start_of_line_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length);

void
editor_buffer_set_cursor_point_to_end_of_line_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length);

void
editor_buffer_add_cursor_at_point(struct editor_buffer_t editor_buffer, int64_t row, int64_t col);

void
editor_buffer_add_cursor_at_point_virtual(struct editor_buffer_t editor_buffer, int64_t row, int64_t col, int64_t virtual_line_length);

void
editor_buffer_clear_cursors(struct editor_buffer_t editor_buffer);

void
editor_buffer_set_cursor_pos_relative(struct editor_buffer_t editor_buffer, int64_t relative_cursor);

int64_t
editor_buffer_get_cursor_selection_start_row_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length);

int64_t
editor_buffer_get_cursor_selection_start_col_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length);

int64_t
editor_buffer_cursor_is_selection(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

void
editor_buffer_set_cursor_is_selection(struct editor_buffer_t editor_buffer, int8_t cursor_is_selection);

int64_t
editor_buffer_get_cursor_selection_start_row(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

int64_t
editor_buffer_get_cursor_selection_start_col(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

int64_t
editor_buffer_get_cursor_selection_start_pos(struct editor_buffer_t editor_buffer, int64_t cursor_idx);

const char *
editor_buffer_get_buf_bytes(struct buf_t *buf);

void
editor_buffer_free_buf(struct buf_t *buf);

void
ensure_virtual_newline_length(struct line_rope_t *rn, int64_t virtual_line_length);

#endif //SE_EDITOR_BUFFER_H
