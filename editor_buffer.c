#include <stdio.h>
#include <string.h>
#include "editor_buffer.h"
#include "rope.h"
#include "circular_buffer.h"
#include "buf.h"
#include "line_rope.h"
#include "stack.h"

void
editor_screen_set_line_and_col_for_char_pos(struct cursor_info_t *cursor_info, struct rope_t *text);

struct editor_buffer_t
editor_buffer_create(uint32_t virtual_line_length)
{
    struct editor_buffer_t editor_buffer;

    editor_buffer.undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), UNDO_BUFFER_SIZE);
    editor_buffer.global_undo_buffer = circular_buffer_init(sizeof(struct editor_screen_t), GLOBAL_UNDO_BUFFER_SIZE);

    editor_buffer.file_path = buf_init(0);

    editor_buffer.undo_idx = se_alloc(1, sizeof(int64_t));
    *editor_buffer.undo_idx = 0;

    editor_buffer.global_undo_idx = se_alloc(1, sizeof(int64_t));
    *editor_buffer.global_undo_idx = 0;

    editor_buffer.save_to_undo = se_alloc(1, sizeof(int8_t));
    *editor_buffer.save_to_undo = 1;

    editor_buffer.current_screen = se_alloc(1, sizeof(struct editor_screen_t));

    struct editor_screen_t screen;

    struct cursor_info_t cursor_info;
    cursor_info.char_pos = 0;
    cursor_info.row = 0;
    cursor_info.col = 0;
    cursor_info.is_selection = 0;

    screen.cursor_infos = vector_init(16, sizeof(struct cursor_info_t));
    vector_append(screen.cursor_infos, &cursor_info);

    screen.text = rope_leaf_init("");

    // todo(chad): initialize this only if the number of lines is less than a certain threshold
    screen.lines = line_rope_leaf_init(0, virtual_line_length);
//    screen.lines = NULL;

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
        rope_dec_rc(screen->text);
        line_rope_dec_rc(screen->lines);
    }

    for (int64_t i = 0; i < editor_buffer.global_undo_buffer->length; i++) {
        struct editor_screen_t *screen = circular_buffer_at(editor_buffer.undo_buffer, i);
        rope_dec_rc(screen->text);
        line_rope_dec_rc(screen->lines);
    }

    free(editor_buffer.undo_idx);
    free(editor_buffer.global_undo_idx);

    free(editor_buffer.current_screen);
}

struct rope_t *
read_file(const char *file_path, struct line_helper_t *line_helper)
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

    struct rope_t *rn = rope_leaf_init_lines(src, line_helper);

    free(src);

    return rn;
}

int64_t
editor_screen_calculate_line_length(struct editor_screen_t screen, int64_t line)
{
    int64_t char_number_at_current_line = rope_char_number_at_line(screen.text, line);
    int64_t char_number_at_next_line = rope_char_number_at_line(screen.text, line + 1);
    int64_t answer = (int64_t) (char_number_at_next_line - char_number_at_current_line - 1);
    if (answer < 0) { answer = 0; }
    return answer;
}

void
editor_screen_balance_lines_if_needed(struct editor_screen_t *screen)
{
    // todo(chad): come up with an actual condition here

    struct line_rope_t *new_lines = line_rope_balance(screen->lines);
    line_rope_free(screen->lines);
    screen->lines = new_lines;
}

void
ensure_virtual_newline_length(struct line_rope_t *rn, int64_t virtual_line_length)
{
    if (rn == NULL) { return; }

    if (rn->virtual_line_length == virtual_line_length) { return; }

    rn->virtual_line_length = (uint32_t) virtual_line_length;

    if (!rn->is_leaf) {
        ensure_virtual_newline_length(rn->left, virtual_line_length);
        ensure_virtual_newline_length(rn->right, virtual_line_length);
    }

    rn->virtual_newline_count = line_rope_virtual_newline_weight(rn);
    rn->total_virtual_newline_count = line_rope_total_virtual_newline_weight(rn);
}

void
editor_buffer_open_file(struct editor_buffer_t editor_buffer, uint32_t virtual_line_length, const char *file_path)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    struct editor_screen_t screen;

    struct cursor_info_t cursor_info;
    cursor_info.char_pos = 0;
    cursor_info.row = 0;
    cursor_info.col = 0;
    cursor_info.is_selection = 0;

    screen.cursor_infos = vector_init(16, sizeof(struct cursor_info_t));
    vector_append(screen.cursor_infos, &cursor_info);

    struct vector_t *lines_vector = vector_init(64, sizeof(int64_t));
    struct line_helper_t line_helper;
    line_helper.lines = lines_vector;
    line_helper.leftover = 0;

    screen.text = read_file(file_path, &line_helper);
    int64_t total_line_lengths = 0;

    struct vector_t *stack = stack_init(sizeof(struct line_rope_t *));

    int64_t line = 0;
    int64_t char_number_at_current_line = 0;

    int64_t char_number_at_next_line;
    if (line >= lines_vector->length) {
        char_number_at_next_line = rope_total_char_length(screen.text) + 1;
    } else {
        int64_t line_length = *(int64_t *) vector_at(lines_vector, line);
        total_line_lengths += line_length;
        char_number_at_next_line = total_line_lengths;
    }

    struct line_rope_t *rn = line_rope_leaf_init(
            (uint32_t) (char_number_at_next_line - char_number_at_current_line - 1),
            virtual_line_length);
    stack_push(stack, &rn);
    line += 1;
    int64_t lines = 1 + rope_total_line_break_length(screen.text);
    while (line < lines) {
        char_number_at_current_line = char_number_at_next_line;

        if (line >= lines_vector->length) {
            char_number_at_next_line = rope_total_char_length(screen.text) + 1;
        } else {
            int64_t line_length = *(int64_t *) vector_at(lines_vector, line);
            total_line_lengths += line_length - 1;
            char_number_at_next_line = total_line_lengths;
        }

        rn = line_rope_leaf_init((uint32_t) (char_number_at_next_line - char_number_at_current_line - 1), virtual_line_length);
        stack_push(stack, &rn);

        int8_t c = 0;
        while (stack->length > 1 && !c) {
            struct line_rope_t *top = (struct line_rope_t *) vector_at_deref(stack, stack->length - 1);
            struct line_rope_t *second = (struct line_rope_t *) vector_at_deref(stack, stack->length - 2);
            if (top->height == second->height) {
                stack_pop(stack);
                stack_pop(stack);

                struct line_rope_t *new_line = line_rope_parent_init(second, top, virtual_line_length);
                stack_push(stack, &new_line);
            } else {
                c = 1;
            }
        }

        line += 1;
    }

    vector_free(lines_vector);

    while (stack->length > 1) {
        struct line_rope_t *top = (struct line_rope_t *) vector_at_deref(stack, stack->length - 1);
        struct line_rope_t *second = (struct line_rope_t *) vector_at_deref(stack, stack->length - 2);
        stack_pop(stack);
        stack_pop(stack);

        struct line_rope_t *new_line = line_rope_parent_init(second, top, rn->virtual_line_length);
        stack_push(stack, &new_line);
    }
    SE_ASSERT(stack->length == 1);
    screen.lines = (struct line_rope_t *) stack_pop_deref(stack);
    vector_free(stack);

    undo_stack_append(editor_buffer, screen);

    // todo(chad): @Leak
    *editor_buffer.file_path = *buf_init_fmt("%str", file_path);
}

int32_t
editor_buffer_save_file(struct editor_buffer_t editor_buffer)
{
    if (editor_buffer.file_path == NULL) {
        fprintf(stderr, "current file path is NULL\n");

        return -1;
    }

    return editor_buffer_save_file_as(editor_buffer, editor_buffer.file_path->bytes);
}

int32_t
editor_buffer_save_file_as(struct editor_buffer_t editor_buffer, const char *file_name)
{
    FILE *file = fopen(file_name, "w");
    if (file == NULL) {
        fprintf(stderr, "error opening file\n");

        return -1;
    }

    *editor_buffer.file_path = *buf_init_fmt("%str", file_name);

    rewind(file);

    struct editor_screen_t screen = editor_buffer_get_current_screen(editor_buffer);

    // todo(chad): should stream the text char by char to the file rather than potentially re-allocating the entire size of the text
    struct buf_t *all_text = buf_init_fmt("%rope", screen.text);
    int32_t err = fprintf(file, "%s", all_text->bytes);
    buf_free(all_text);

    fclose(file);

    return err;
}

int8_t
editor_buffer_has_file_path(struct editor_buffer_t editor_buffer)
{
    return editor_buffer.file_path->length > 0;
}

struct cursor_info_t *
possibly_merge_cursors(struct cursor_info_t *i, struct cursor_info_t *j)
{
    if (i->is_selection && j->is_selection) {
        int64_t i_end_selection = i->char_pos;
        int64_t j_start_selection = j->selection_char_pos;

        int8_t i_is_forward = 0;
        int8_t j_is_forward = 0;

        if (i->char_pos < i->selection_char_pos) {
            i_is_forward = 1;
            i_end_selection = i->selection_char_pos;
        }

        if (j->char_pos < j->selection_char_pos) {
            j_is_forward = 1;
            j_start_selection = j->char_pos;
        }

        if (i_end_selection <= j_start_selection) {
            return NULL;
        }

        struct cursor_info_t *info = se_alloc(1, sizeof(struct cursor_info_t));
        info->is_selection = 1;

        if (j_is_forward) {
            // I J i j
            // cursor: i, selection: j_selection
            info->char_pos = i->char_pos;
            info->row = i->row;
            info->col = i->col;

            info->selection_char_pos = j->selection_char_pos;
            info->selection_row = j->selection_row;
            info->selection_col = j->selection_col;
        } else {
            // i j I J
            // cursor: j, selection: i_selection
            info->char_pos = j->char_pos;
            info->row = j->row;
            info->col = j->col;

            info->selection_char_pos = i->selection_char_pos;
            info->selection_row = i->selection_row;
            info->selection_col = i->selection_col;
        }

        return info;
    } else if (i->is_selection) {
        if (i->char_pos <= j->char_pos && i->selection_char_pos >= j->char_pos) {
            // I J i
            return i;
        } else if (i->char_pos >= j->char_pos && i->selection_char_pos <= j->char_pos) {
            // i J I
            return i;
        } else {
            return NULL;
        }
    } else if (j->is_selection) {
        if (j->char_pos <= i->char_pos && j->selection_char_pos >= i->char_pos) {
            // J I j
            return j;
        } else if (j->char_pos >= i->char_pos && j->selection_char_pos <= i->char_pos) {
            // j I J
            return j;
        }
    } else {
        // I == J
        if (i->char_pos == j->char_pos) {
            return i;
        } else {
            return NULL;
        }
    }

    return NULL;
}

void
merge_cursors(struct vector_t *cursor_infos)
{
    if (cursor_infos->length <= 1) {
        return;
    }

    for (int64_t i = 0; i < cursor_infos->length - 1; i++) {
        struct cursor_info_t *cursor_i = (struct cursor_info_t *) vector_at(cursor_infos, i);
        struct cursor_info_t *cursor_i_1 = (struct cursor_info_t *) vector_at(cursor_infos, i + 1);

        struct cursor_info_t *merged = possibly_merge_cursors(cursor_i, cursor_i_1);
        if (merged != NULL) {
            vector_set_at(cursor_infos, i, merged);

            if (merged != cursor_i && merged != cursor_i_1) {
                free(merged);
            }

            if (cursor_infos->length > i + 2) {
                memmove(cursor_infos->buf + (i + 1) * cursor_infos->item_size,
                        cursor_infos->buf + (i + 2) * cursor_infos->item_size,
                        (cursor_infos->length - (i + 2)) * cursor_infos->item_size);
            }

            cursor_infos->length -= 1;
            i -= 1;
        }
    }
}

void
sort_and_merge_cursors(struct editor_buffer_t editor_buffer)
{
    // insertion sort
    struct vector_t *cursor_infos = editor_buffer.current_screen->cursor_infos;

    if (cursor_infos->length == 1) { return; }

    int64_t i = 1;
    while (i < cursor_infos->length) {
        int64_t j = i;

        // while (j > 0 && A[j - 1] > A[j])
        while (j > 0 &&
               ((struct cursor_info_t *) vector_at(cursor_infos, j - 1))->char_pos >
               ((struct cursor_info_t *) vector_at(cursor_infos, j))->char_pos) {
            struct cursor_info_t c_a_j_1 = *(struct cursor_info_t *) vector_at(cursor_infos, j - 1);
            struct cursor_info_t c_a_j = *(struct cursor_info_t *) vector_at(cursor_infos, j);

            // swap items at indices [j] and [j - 1]
            vector_set_at(cursor_infos, j, &c_a_j_1);
            vector_set_at(cursor_infos, j - 1, &c_a_j);

            j = j - 1;
        }
        i = i + 1;
    }

    merge_cursors(cursor_infos);
}

void
editor_buffer_delete_possibly_only_selection(struct editor_buffer_t editor_buffer, int8_t should_delete_non_selection)
{
    struct editor_screen_t edited_screen;
    edited_screen.cursor_infos = vector_copy(editor_buffer.current_screen->cursor_infos);
    edited_screen.lines = line_rope_shallow_copy(editor_buffer.current_screen->lines);
    edited_screen.text = rope_shallow_copy(editor_buffer.current_screen->text);

    for (int64_t i = edited_screen.cursor_infos->length - 1; i >= 0; i--) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(edited_screen.cursor_infos, i);

        if (cursor_info->char_pos == 0 && !cursor_info->is_selection) {
            cursor_info->selection_char_pos = 0;
            cursor_info->selection_row = 0;
            continue;
        }

        int64_t start_row;

        int64_t delete_from;
        int64_t delete_to;
        if (cursor_info->is_selection) {
            int64_t cursor = cursor_info->char_pos;
            int64_t selection = cursor_info->selection_char_pos;

            if (cursor < selection) {
                delete_from = cursor;
                delete_to = selection;

                start_row = cursor_info->selection_row;
            } else {
                delete_from = selection;
                delete_to = cursor;

                start_row = cursor_info->row;
            }
        } else {
            delete_from = cursor_info->char_pos - 1;
            delete_to = cursor_info->char_pos;

            start_row = cursor_info->row;
        }

        if (!should_delete_non_selection && !cursor_info->is_selection) {
            cursor_info->selection_char_pos = 0;
            continue;
        }
        
        if (delete_from == delete_to) {
            return;
        }

        struct rope_t *edited = rope_delete(edited_screen.text, delete_from, delete_to);
        if (edited == NULL) {
            // we deleted everything!!
            edited = rope_leaf_init("");
        }

        rope_free(edited_screen.text);
        edited_screen.text = edited;

        if (cursor_info->is_selection) {
            cursor_info->is_selection = 0;

            int64_t diff = cursor_info->selection_char_pos - cursor_info->char_pos;
            if (diff < 0) { diff = -diff; }

            if (cursor_info->selection_char_pos > cursor_info->char_pos) {
                cursor_info->char_pos = cursor_info->selection_char_pos;
            }

            // note(chad):
            // this is a clever hack, which probably means it's bad.
            // basically since selection_char_pos is no longer used, we can put stuff there
            // this saves allocations :)
            cursor_info->selection_char_pos = diff;
        } else {
            cursor_info->selection_char_pos = 1;
        }

        // more hackery
        cursor_info->selection_row = start_row;
    }

    int64_t total = 0;
    int64_t lines_deleted = 0;
    for (int64_t i = 0; i < edited_screen.cursor_infos->length; i++) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(edited_screen.cursor_infos, i);
        total += cursor_info->selection_char_pos;
        cursor_info->char_pos -= total;
        if (cursor_info->char_pos < 0) {
            cursor_info->char_pos = 0;
        }

        editor_screen_set_line_and_col_for_char_pos(cursor_info, edited_screen.text);

        int64_t end_row = cursor_info->row;
        if (end_row + lines_deleted != cursor_info->selection_row) {
            struct line_rope_t *saved_lines = edited_screen.lines;
            
            SE_ASSERT(end_row + lines_deleted < cursor_info->selection_row);
            edited_screen.lines = line_rope_delete(edited_screen.lines, end_row + lines_deleted, cursor_info->selection_row);
            line_rope_free(saved_lines);

            lines_deleted = cursor_info->selection_row - end_row;
            int64_t new_line_length = editor_screen_calculate_line_length(edited_screen, end_row);

            saved_lines = edited_screen.lines;
            edited_screen.lines = line_rope_replace_char_at(edited_screen.lines, end_row, new_line_length);
            line_rope_free(saved_lines);
            
            int64_t total_lines = 1 + rope_total_line_break_length(edited_screen.text);
            if (end_row + 1 < total_lines) {
                new_line_length = editor_screen_calculate_line_length(edited_screen, end_row + 1);

                saved_lines = edited_screen.lines;
                edited_screen.lines = line_rope_replace_char_at(edited_screen.lines, end_row + 1, new_line_length);
                line_rope_free(saved_lines);
            }
        } else {
            int64_t new_line_length = editor_screen_calculate_line_length(edited_screen, end_row);

            struct line_rope_t *saved_lines = edited_screen.lines;
            edited_screen.lines = line_rope_replace_char_at(edited_screen.lines, end_row, new_line_length);
            line_rope_free(saved_lines);
        }
    }

    undo_stack_append(editor_buffer, edited_screen);

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_delete(struct editor_buffer_t editor_buffer)
{
    editor_buffer_delete_possibly_only_selection(editor_buffer, 1);
}

int64_t
editor_buffer_character_position_for_virtual_point(struct editor_buffer_t editor_buffer,
                                                   int64_t line,
                                                   int64_t col,
                                                  int64_t virtual_line_length);

int64_t
editor_buffer_character_position_for_point(struct editor_buffer_t editor_buffer,
                                           int64_t line,
                                           int64_t col);

void
editor_buffer_delete_at_point(struct editor_buffer_t editor_buffer, int64_t num_chars,
                              int64_t row, int64_t col,
                              int8_t virtual, int64_t virtual_line_length)
{
    if (num_chars <= 0) { return; }

    struct editor_screen_t edited_screen;

    edited_screen.cursor_infos = vector_init(1, sizeof(struct cursor_info_t));
    struct vector_t *old_cursor_infos = vector_copy(editor_buffer.current_screen->cursor_infos);

    edited_screen.lines = line_rope_shallow_copy(editor_buffer.current_screen->lines);
    edited_screen.text = rope_shallow_copy(editor_buffer.current_screen->text);

    struct cursor_info_t cursor_info;
    cursor_info.is_selection = 0;
    if (virtual) {
        cursor_info.char_pos = editor_buffer_character_position_for_virtual_point(editor_buffer, row, col, virtual_line_length);
    } else {
        cursor_info.char_pos = editor_buffer_character_position_for_point(editor_buffer, row, col);
    }

    editor_screen_set_line_and_col_for_char_pos(&cursor_info, edited_screen.text);

    cursor_info.selection_char_pos = cursor_info.char_pos;
    cursor_info.selection_row = cursor_info.row;
    cursor_info.selection_col = cursor_info.col;

    cursor_info.is_selection = 1;
    cursor_info.char_pos -= num_chars;
    if (cursor_info.char_pos < 0) { cursor_info.char_pos = 0; }

    editor_screen_set_line_and_col_for_char_pos(&cursor_info, edited_screen.text);

    vector_append(edited_screen.cursor_infos, &cursor_info);

    *editor_buffer.current_screen = edited_screen;
    editor_buffer_delete(editor_buffer);

    free(edited_screen.cursor_infos);

    (*editor_buffer.current_screen).cursor_infos = old_cursor_infos;
    for (int64_t i = 0; i < old_cursor_infos->length; i++) {
        struct cursor_info_t *ci = (struct cursor_info_t *) vector_at(old_cursor_infos, i);
        if (ci->char_pos > cursor_info.char_pos) {
            ci->char_pos -= num_chars;
            if (ci->char_pos < 0) { ci->char_pos = 0; }
            editor_screen_set_line_and_col_for_char_pos(ci, editor_buffer.current_screen->text);
        }
    }
}

void
editor_buffer_insert(struct editor_buffer_t editor_buffer, const char *text)
{
    int8_t exists_selection = 0;
    for (int64_t i = editor_buffer.current_screen->cursor_infos->length - 1; i >= 0; i--) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i);
        if (cursor_info->is_selection) {
            exists_selection = 1;
            break;
        }
    }
    if (exists_selection) {
        // if we're inserting when there's a selection, delete that first
        editor_buffer_delete_possibly_only_selection(editor_buffer, 0);
    }

    struct editor_screen_t edited_screen;
    edited_screen.cursor_infos = vector_copy(editor_buffer.current_screen->cursor_infos);
    edited_screen.lines = line_rope_shallow_copy(editor_buffer.current_screen->lines);
    edited_screen.text = rope_shallow_copy(editor_buffer.current_screen->text);

    for (int64_t i = edited_screen.cursor_infos->length - 1; i >= 0; i--) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(edited_screen.cursor_infos, i);

        int64_t cursor_line_before = cursor_info->row;

        struct rope_t *edited = rope_insert(edited_screen.text, cursor_info->char_pos, text);
        rope_free(edited_screen.text);
        edited_screen.text = edited;

        // todo(chad): @Performance
        // count_newlines might take a while in some cases?
        // plus we already had to do it inside the rope_insert() above
        // could find a way to preserve that value so we don't do it twice
        int64_t cursor_line_after = cursor_info->row + count_newlines(text);

        int64_t line = cursor_line_before;
        int64_t line_length = editor_screen_calculate_line_length(edited_screen, line);

        struct line_rope_t *saved_lines = edited_screen.lines;
        edited_screen.lines = line_rope_replace_char_at(edited_screen.lines,
                                                        line,
                                                        line_length);
        line_rope_free(saved_lines);

        line += 1;
        while (line <= cursor_line_after) {
            line_length = editor_screen_calculate_line_length(edited_screen, line);

            struct line_rope_t *new_lines = line_rope_insert(edited_screen.lines,
                                                             line,
                                                             line_length);
            line_rope_free(edited_screen.lines);
            edited_screen.lines = new_lines;

            line += 1;
        }
    }

    for (int64_t i = edited_screen.cursor_infos->length - 1; i >= 0; i--) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(edited_screen.cursor_infos, i);
        // position cursor at the end of the insertion
        cursor_info->char_pos += (i + 1) * unicode_strlen(text);
        editor_screen_set_line_and_col_for_char_pos(cursor_info, edited_screen.text);
    }

    undo_stack_append(editor_buffer, edited_screen);
}

int64_t
editor_buffer_character_position_for_point(struct editor_buffer_t editor_buffer,
                                                   int64_t line,
                                                   int64_t col);

int64_t
editor_buffer_character_position_for_virtual_point(struct editor_buffer_t editor_buffer,
                                                   int64_t line,
                                                   int64_t col,
                                                   int64_t virtual_line_length);

void
editor_buffer_insert_at_point(struct editor_buffer_t editor_buffer, const char *text,
                              int64_t row, int64_t col,
                              int8_t virtual, int64_t virtual_line_length)
{
    struct editor_screen_t edited_screen = *editor_buffer.current_screen;
    edited_screen.cursor_infos = vector_copy(editor_buffer.current_screen->cursor_infos);
    edited_screen.lines = line_rope_shallow_copy(editor_buffer.current_screen->lines);
    edited_screen.text = rope_shallow_copy(editor_buffer.current_screen->text);

    int64_t cursor_line_before = row;

    int64_t char_pos;
    if (virtual) {
        char_pos = editor_buffer_character_position_for_virtual_point(editor_buffer, row, col, virtual_line_length);
    } else {
        char_pos = editor_buffer_character_position_for_point(editor_buffer, row, col);
    }

    struct rope_t *edited = rope_insert(edited_screen.text, char_pos, text);
    rope_free(edited_screen.text);
    edited_screen.text = edited;

    // todo(chad): @Performance
    // count_newlines might take a while in some cases?
    // plus we already had to do it inside the rope_insert() above
    // could find a way to preserve that value so we don't do it twice
    int64_t cursor_line_after = row + count_newlines(text);

    int64_t line = cursor_line_before;
    int64_t line_length = editor_screen_calculate_line_length(edited_screen, line);

    struct line_rope_t *saved_lines = edited_screen.lines;
    edited_screen.lines = line_rope_replace_char_at(edited_screen.lines,
                                                    line,
                                                    line_length);
    line_rope_free(saved_lines);

    line += 1;
    while (line <= cursor_line_after) {
        line_length = editor_screen_calculate_line_length(edited_screen, line);

        struct line_rope_t *new_lines = line_rope_insert(edited_screen.lines,
                                                         line,
                                                         line_length);
        line_rope_free(edited_screen.lines);
        edited_screen.lines = new_lines;

        line += 1;
    }

    for (int64_t i = edited_screen.cursor_infos->length - 1; i >= 0; i--) {
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(edited_screen.cursor_infos, i);

        if (cursor_info->char_pos >= char_pos) {
            // advance cursor by length of the insertion
            cursor_info->char_pos += unicode_strlen(text);
            editor_screen_set_line_and_col_for_char_pos(cursor_info, edited_screen.text);
        }
    }

    undo_stack_append(editor_buffer, edited_screen);
}

void
editor_buffer_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    int64_t computed_index = undo_idx;
    if (computed_index > editor_buffer.undo_buffer->length - 1) {
        computed_index = editor_buffer.undo_buffer->length - 1;
    } if (computed_index < 0) {
        computed_index = 0;
    }

    if (computed_index == *editor_buffer.undo_idx) { return; }

    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.undo_buffer, computed_index);
    *editor_buffer.current_screen = *undone_screen;

    *editor_buffer.undo_idx = computed_index;
    *editor_buffer.global_undo_idx = editor_buffer.global_undo_buffer->length - 1;
}

void
editor_buffer_global_undo(struct editor_buffer_t editor_buffer, int64_t undo_idx)
{
    int64_t computed_index = undo_idx;
    if (computed_index > editor_buffer.global_undo_buffer->length - 1) {
        computed_index = editor_buffer.global_undo_buffer->length - 1;
    } if (computed_index < 0) {
        computed_index = 0;
    }

    if (computed_index == *editor_buffer.global_undo_idx) { return; }

    struct editor_screen_t *undone_screen = circular_buffer_at(editor_buffer.global_undo_buffer, computed_index);

    *editor_buffer.global_undo_idx = computed_index;
    *editor_buffer.undo_idx = editor_buffer.undo_buffer->length - 1;
    *editor_buffer.current_screen = *undone_screen;
}

int64_t
editor_buffer_get_line_count(struct editor_buffer_t editor_buffer)
{
    return 1 + rope_total_line_break_length(editor_buffer.current_screen->text);
}

int64_t
line_rope_char_at_virtual_newline(struct line_rope_t *rn, int64_t i)
{
    if (rn == NULL) { return -1; }

    if (rn->is_leaf) {
        if ((int64_t) rn->virtual_newline_count - 1 < i) {
            return 0;
        }

        return 0;
    }

    if ((int64_t) rn->virtual_newline_count - 1 < i) {
        return rn->char_weight + line_rope_char_at_virtual_newline(rn->right, i - rn->virtual_newline_count);
    } else {
        if (rn->left != NULL) {
            return line_rope_char_at_virtual_newline(rn->left, i);
        }
        return 0;
    }
}

int64_t
virtual_newline_total(struct line_rope_t *rn, int64_t line)
{
    if (rn == NULL) { return -1; }

    if (rn->is_leaf) {
        if ((int64_t) rn->char_weight - 1 < line) {
            return 0;
        }

        return rn->total_virtual_newline_count;
    }

    if ((int64_t) rn->char_weight - 1 < line) {
        return rn->virtual_newline_count + virtual_newline_total(rn->right, line - rn->char_weight);
    } else {
        if (rn->left != NULL) {
            return virtual_newline_total(rn->left, line);
        }
        return 0;
    }
}

int64_t
editor_buffer_character_position_for_point(struct editor_buffer_t editor_buffer,
                                           int64_t line,
                                           int64_t col)
{
    int64_t computed_row = line;
    int64_t row_count = editor_buffer_get_line_count(editor_buffer);
    if (computed_row > row_count - 1) { computed_row = row_count - 1; }
    if (computed_row < 0) { computed_row = 0; }

    int64_t start_for_row = editor_buffer_get_char_number_at_line(editor_buffer, computed_row);
    int64_t end_of_row = editor_buffer_get_end_of_row(editor_buffer, computed_row);
    int64_t row_length = end_of_row - start_for_row;

    int64_t computed_col = row_length < col ? row_length : col;

    return start_for_row + computed_col;
}

int64_t
editor_buffer_character_position_for_virtual_point(struct editor_buffer_t editor_buffer,
                                                   int64_t line,
                                                   int64_t col,
                                                  int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t current_line = line_rope_char_at_virtual_newline(editor_buffer.current_screen->lines, line);
    int64_t current_line_length = editor_buffer_get_line_length(editor_buffer, current_line);

    int64_t virtual_newlines_before_current_line = 0;
    if (current_line > 0) {
        virtual_newlines_before_current_line = virtual_newline_total(editor_buffer.current_screen->lines, current_line - 1);
    }

    int64_t char_pos = line_rope_char_number_at_line(editor_buffer.current_screen->lines, current_line);
    int64_t additional_virtual_newlines_needed = line - virtual_newlines_before_current_line;
    int64_t additional_cols_needed = additional_virtual_newlines_needed * virtual_line_length;

    int64_t final_char_pos = char_pos + additional_cols_needed + col;

    int64_t char_pos_at_start_of_current_line = line_rope_char_number_at_line(editor_buffer.current_screen->lines, current_line);

    // make sure it doesn't go past the end of the current line
    int64_t end_of_current_line = char_pos_at_start_of_current_line + current_line_length;
    if (final_char_pos > end_of_current_line) {
        final_char_pos = end_of_current_line;
    }

    return final_char_pos;
}

int64_t
editor_buffer_get_line_length(struct editor_buffer_t editor_buffer, int64_t line)
{
    struct line_rope_t *found_line = line_rope_char_at(editor_buffer.current_screen->lines, line);
    int64_t found_line_length = found_line == NULL ? -1 : (int64_t) found_line->line_length;

    return found_line_length;
}

int64_t
editor_buffer_get_line_length_virtual(struct editor_buffer_t editor_buffer, int64_t line, int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t computed_start_char = editor_buffer_character_position_for_virtual_point(editor_buffer, line, 0, virtual_line_length);
    int64_t computed_end_char = editor_buffer_character_position_for_virtual_point(editor_buffer, line + 1, 0, virtual_line_length);

    int64_t length = computed_end_char - computed_start_char;
    if (length < 1) { length = 1; }

    return length;
}

int64_t
editor_buffer_get_line_count_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    return editor_buffer.current_screen->lines->total_virtual_newline_count;
}

int64_t
editor_buffer_get_char_count(struct editor_buffer_t editor_buffer)
{
    return rope_total_char_length(editor_buffer.current_screen->text);
}

int64_t
editor_buffer_get_char_number_at_line(struct editor_buffer_t editor_buffer, int64_t i)
{
    struct editor_screen_t screen = editor_buffer_get_current_screen(editor_buffer);
    return line_rope_char_number_at_line(screen.lines, i);
}

//int64_t
//editor_buffer_add_char_incremental(struct rope_t *rn, int64_t start, int64_t end, struct buf_t *buf)
//{
//    if (rn == NULL) { return 0; }
//
//    if (rn->is_leaf) {
//        if (rn->char_weight - 1 < start) {
//            return 0;
//        }
//
//        int64_t byte_offset = 0;
//
//        for (int64_t j = 0; j < start; j++) {
//            byte_offset += bytes_in_codepoint_utf8(*(rn->str_buf->bytes + byte_offset));
//        }
//
//        // write as many chars as we can
//        int64_t j = start;
//        while (j < end && byte_offset < rn->str_buf->length) {
//            int64_t char_size = bytes_in_codepoint_utf8(*(rn->str_buf->bytes + byte_offset));
//            buf_write_bytes(buf, rn->str_buf->bytes + byte_offset, char_size);
//            byte_offset += char_size;
//            j += 1;
//        }
//
//        // return how many chars we wrote
//        return j - start;
//    }
//
//    if (rn->char_weight - 1 < start) {
//        return editor_buffer_add_char_incremental(rn->right, start - rn->char_weight, end - rn->char_weight, buf);
//    } else {
//        if (rn->left != NULL) {
//            int64_t added_from_left = editor_buffer_add_char_incremental(rn->left, start, end, buf);
//            if (rn->right != NULL && start + added_from_left < end) {
//                int64_t added_from_right = editor_buffer_add_char_incremental(rn->right, start + added_from_left - rn->char_weight, end - rn->char_weight, buf);
//                return added_from_left + added_from_right;
//            }
//            return added_from_left;
//        }
//        return 0;
//    }
//}

int64_t
editor_buffer_add_bytes_incremental(struct rope_t *rn, int64_t start, int64_t end, struct buf_t *buf)
{
    if (rn == NULL) { return 0; }

    if (rn->is_leaf) {
        if (rn->byte_weight - 1 < start) {
            return 0;
        }

        int64_t possible_bytes = rn->byte_weight - start;
        int64_t desired_bytes = end - start;
        int64_t actual_bytes = possible_bytes;
        if (actual_bytes > desired_bytes) {
            actual_bytes = desired_bytes;
        }

        // write to the buffer
        buf_write_bytes(buf, rn->str_buf->bytes + start, actual_bytes);

        // return how many chars we wrote
        return actual_bytes;
    }

    if (rn->byte_weight - 1 < start) {
        return editor_buffer_add_bytes_incremental(rn->right, start - rn->byte_weight, end - rn->byte_weight, buf);
    } else {
        if (rn->left != NULL) {
            int64_t added_from_left = editor_buffer_add_bytes_incremental(rn->left, start, end, buf);
            if (rn->right != NULL && start + added_from_left < end) {
                int64_t added_from_right = editor_buffer_add_bytes_incremental(rn->right, start + added_from_left - rn->byte_weight, end - rn->byte_weight, buf);
                return added_from_left + added_from_right;
            }
            return added_from_left;
        }
        return 0;
    }
}

struct buf_t *
editor_buffer_get_text_between_characters(struct editor_buffer_t editor_buffer, int64_t start, int64_t end)
{
    int64_t char_count = editor_buffer_get_char_count(editor_buffer);
    int64_t real_end = end;
    if (real_end > char_count) { real_end = char_count; }

    if (real_end <= start) { return buf_init(0); }

    struct rope_t *rn = editor_buffer.current_screen->text;
    struct buf_t *buf = buf_init(end - start + 1);

    int64_t start_byte = byte_for_char_at(editor_buffer.current_screen->text, start);
    int64_t end_byte = byte_for_char_at(editor_buffer.current_screen->text, end);
    editor_buffer_add_bytes_incremental(rn, start_byte, end_byte, buf);

    return buf;
}

struct buf_t *
editor_buffer_get_text_between_points(struct editor_buffer_t editor_buffer,
                                      int64_t start_line,
                                      int64_t start_col,
                                      int64_t end_line,
                                      int64_t end_col)
{
    int64_t start_line_char_number = line_rope_char_number_at_line(editor_buffer.current_screen->lines, start_line);
    if (start_line_char_number < 0) { start_line_char_number = 0; }

    int64_t end_line_char_number;
    int64_t total_lines = 1 + rope_total_line_break_length(editor_buffer.current_screen->text);

    // if we're asking for something past the last line then there's nothing to do
    if (start_line >= total_lines) { return buf_init(0); }

    if (end_line >= total_lines) {
        end_line_char_number = rope_total_char_length(editor_buffer.current_screen->text);
    } else {
        end_line_char_number = line_rope_char_number_at_line(editor_buffer.current_screen->lines, end_line);
    }

    return editor_buffer_get_text_between_characters(editor_buffer,
                                                     start_line_char_number + start_col,
                                                     end_line_char_number + end_col);
}

struct buf_t *
editor_buffer_get_text_between_points_virtual(struct editor_buffer_t editor_buffer,
                                              int64_t start_line, int64_t start_col,
                                              int64_t end_line, int64_t end_col,
                                              int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t computed_start_char = editor_buffer_character_position_for_virtual_point(editor_buffer,
                                                                                     start_line,
                                                                                     start_col,
                                                                                     virtual_line_length);
    int64_t computed_end_char = editor_buffer_character_position_for_virtual_point(editor_buffer,
                                                                                   end_line,
                                                                                   end_col,
                                                                                   virtual_line_length);

    const char *computed_start = rope_char_at(editor_buffer.current_screen->text, computed_start_char);
    if (computed_start != NULL && *computed_start == '\n') {
        computed_start_char += 1;
    }

    const char *one_before_computed_end = rope_char_at(editor_buffer.current_screen->text, computed_end_char - 1);
    if (one_before_computed_end != NULL && *one_before_computed_end == '\n') {
        computed_end_char -= 1;
    }

    return editor_buffer_get_text_between_characters(editor_buffer, computed_start_char, computed_end_char);
}

int64_t
editor_buffer_get_cursor_pos(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    return cursor_info->char_pos;
}

int64_t
editor_buffer_get_cursor_row(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    return cursor_info->row;
}

int64_t
editor_buffer_get_cursor_col(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    return cursor_info->col;
}

int64_t
editor_buffer_get_virtual_cursor_row_for_point(struct editor_buffer_t editor_buffer,
                                               int64_t row,
                                               int64_t col,
                                              int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t virtual_newlines_before_current_line;
    if (row > 0) {
        virtual_newlines_before_current_line = virtual_newline_total(editor_buffer.current_screen->lines, row - 1);
    } else {
        virtual_newlines_before_current_line = 0;
    }

    int64_t extra_from_cols;
    extra_from_cols = col / virtual_line_length;

    int64_t virtual_row = virtual_newlines_before_current_line + extra_from_cols;

    int64_t current_line_length = editor_buffer_get_line_length(editor_buffer, row);
    if (col != 0 && col % current_line_length == 0 && col % virtual_line_length == 0) {
        return virtual_row - 1;
    }

    return virtual_row;
}

int64_t
editor_buffer_get_cursor_row_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    return editor_buffer_get_virtual_cursor_row_for_point(editor_buffer,
                                                          cursor_info->row,
                                                          cursor_info->col,
                                                          virtual_line_length);
}


int64_t editor_buffer_get_virtual_cursor_col_for_point(struct editor_buffer_t editor_buffer,
                                                       int64_t row,
                                                       int64_t col,
                                                      int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t current_line_length = editor_buffer_get_line_length(editor_buffer, row);
    if (col != 0 && col % current_line_length == 0 && col % virtual_line_length == 0) {
        return current_line_length;
    }

    return col % virtual_line_length;
}

int64_t
editor_buffer_get_cursor_col_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    return editor_buffer_get_virtual_cursor_col_for_point(editor_buffer,
                                                          cursor_info->row,
                                                          cursor_info->col,
                                                          virtual_line_length);
}

void
editor_buffer_set_cursor_point_virtual_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t row, int64_t col, int64_t virtual_line_length)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t char_pos = editor_buffer_character_position_for_virtual_point(editor_buffer, row, col, virtual_line_length);
    int64_t total_length = rope_total_char_length(editor_buffer.current_screen->text);
    if (char_pos > total_length) {
        char_pos = total_length;
    }

    editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, cursor_idx, char_pos);

//    if (cursor_info->char_pos == cursor_info->selection_char_pos) {
//        cursor_info->is_selection = 0;
//    }
}

void
editor_buffer_set_cursor_point_virtual(struct editor_buffer_t editor_buffer, int64_t row, int64_t col, int64_t virtual_line_length)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_virtual_for_cursor_index(editor_buffer, i, row, col, virtual_line_length);
    }

    sort_and_merge_cursors(editor_buffer);
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

int64_t
editor_buffer_get_cursor_count(struct editor_buffer_t editor_buffer)
{
    return editor_buffer.current_screen->cursor_infos->length;
}

void
editor_buffer_set_cursor_pos_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t cursor)
{
    if (cursor_idx >= editor_buffer.current_screen->cursor_infos->length) { return; }

    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    cursor_info->char_pos = cursor;

    if (cursor_info->char_pos < 0) {
        cursor_info->char_pos = 0;
    }

    int64_t total_length = rope_total_char_length(editor_buffer.current_screen->text);
    if (cursor_info->char_pos > total_length) {
        cursor_info->char_pos = total_length;
    }

    editor_screen_set_line_and_col_for_char_pos(cursor_info, editor_buffer.current_screen->text);
}

void
editor_buffer_set_cursor_pos(struct editor_buffer_t editor_buffer, int64_t cursor)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, cursor);
    }

    sort_and_merge_cursors(editor_buffer);
}

int64_t
editor_buffer_get_end_of_row(struct editor_buffer_t editor_buffer, int64_t row)
{
    int64_t num_lines = editor_buffer_get_line_count(editor_buffer);
    if (row + 1 >= num_lines) {
        return editor_buffer_get_char_count(editor_buffer);
    }

    int64_t start_of_next_line = editor_buffer_get_char_number_at_line(editor_buffer, row + 1);
    return start_of_next_line - 1;
}

void
editor_buffer_set_cursor_point_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_index, int64_t row, int64_t col)
{
    if (cursor_index >= editor_buffer.current_screen->cursor_infos->length) { return; }

    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_index);

    int64_t computed_row = row;
    int64_t row_count = editor_buffer_get_line_count(editor_buffer);
    if (computed_row > row_count - 1) { computed_row = row_count - 1; }
    if (computed_row < 0) { computed_row = 0; }

    int64_t start_for_row = editor_buffer_get_char_number_at_line(editor_buffer, computed_row);
    int64_t end_of_row = editor_buffer_get_end_of_row(editor_buffer, computed_row);
    int64_t row_length = end_of_row - start_for_row;

    int64_t computed_col = row_length < col ? row_length : col;
    if (computed_col < 0) { computed_col = 0; }

    cursor_info->char_pos = start_for_row + computed_col;
    cursor_info->row = computed_row;
    cursor_info->col = computed_col;
}

void
editor_buffer_set_cursor_point(struct editor_buffer_t editor_buffer, int64_t row, int64_t col)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_for_cursor_index(editor_buffer, i, row, col);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_set_cursor_point_to_start_of_line_for_cursor_index(struct editor_buffer_t editor_buffer,
                                                                 int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
    cursor_info->char_pos -= cursor_info->col;
    cursor_info->col = 0;
}

void
editor_buffer_set_cursor_point_to_start_of_line(struct editor_buffer_t editor_buffer)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_to_start_of_line_for_cursor_index(editor_buffer, i);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_set_cursor_point_to_end_of_line_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);
        int64_t line_length = editor_buffer_get_line_length(editor_buffer, cursor_info->row);
        cursor_info->char_pos += (line_length - cursor_info->col);
        cursor_info->col = line_length;
}

void
editor_buffer_set_cursor_point_to_end_of_line(struct editor_buffer_t editor_buffer)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_to_end_of_line_for_cursor_index(editor_buffer, i);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_set_cursor_point_to_start_of_line_virtual_for_cursor_index(struct editor_buffer_t editor_buffer,
                                                                         int64_t cursor_idx,
                                                                         int64_t virtual_line_length)
{
    int64_t row = editor_buffer_get_cursor_row_virtual(editor_buffer, cursor_idx, virtual_line_length);
    editor_buffer_set_cursor_point_virtual_for_cursor_index(editor_buffer, cursor_idx, row, 0, virtual_line_length);
}

void
editor_buffer_set_cursor_point_to_start_of_line_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_to_start_of_line_virtual_for_cursor_index(editor_buffer, i, virtual_line_length);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_set_cursor_point_to_end_of_line_virtual_for_cursor_index(struct editor_buffer_t editor_buffer,
                                                                       int64_t cursor_idx,
                                                                       int64_t virtual_line_length)
{
    int64_t row = editor_buffer_get_cursor_row_virtual(editor_buffer, cursor_idx, virtual_line_length);
    int64_t line_length = editor_buffer_get_line_length_virtual(editor_buffer, row, virtual_line_length);
    editor_buffer_set_cursor_point_virtual_for_cursor_index(editor_buffer, cursor_idx, row, line_length, virtual_line_length);
}

void
editor_buffer_set_cursor_point_to_end_of_line_virtual(struct editor_buffer_t editor_buffer, int64_t virtual_line_length)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_point_to_end_of_line_virtual_for_cursor_index(editor_buffer, i, virtual_line_length);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_add_cursor_at_point(struct editor_buffer_t editor_buffer, int64_t row, int64_t col)
{
    int64_t computed_row = row;
    int64_t row_count = editor_buffer_get_line_count(editor_buffer);
    if (computed_row > row_count - 1) { computed_row = row_count - 1; }
    if (computed_row < 0) { computed_row = 0; }

    int64_t start_for_row = editor_buffer_get_char_number_at_line(editor_buffer, computed_row);
    int64_t end_of_row = editor_buffer_get_end_of_row(editor_buffer, computed_row);
    int64_t row_length = end_of_row - start_for_row;

    int64_t computed_col = row_length < col ? row_length : col;

    struct cursor_info_t cursor_info;
    cursor_info.is_selection = 0;
    cursor_info.char_pos = start_for_row + computed_col;
    cursor_info.row = computed_row;
    cursor_info.col = computed_col;

    vector_append(editor_buffer.current_screen->cursor_infos, &cursor_info);

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_add_cursor_at_point_virtual(struct editor_buffer_t editor_buffer, int64_t row, int64_t col, int64_t virtual_line_length)
{
    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    int64_t char_pos = editor_buffer_character_position_for_virtual_point(editor_buffer, row, col, virtual_line_length);
    int64_t total_length = rope_total_char_length(editor_buffer.current_screen->text);
    if (char_pos > total_length) {
        char_pos = total_length;
    }

    int64_t logical_x = rope_get_line_number_for_char_pos(editor_buffer.current_screen->text, char_pos);

    int64_t row_start = rope_char_number_at_line(editor_buffer.current_screen->text, logical_x);
    int64_t logical_y = char_pos - row_start;

    editor_buffer_add_cursor_at_point(editor_buffer, logical_x, logical_y);
}

void
editor_buffer_clear_cursors(struct editor_buffer_t editor_buffer)
{
    editor_buffer.current_screen->cursor_infos->length = 0;
}

void
editor_buffer_make_single_cursor(struct editor_buffer_t editor_buffer)
{
    editor_buffer.current_screen->cursor_infos->length = 1;
}

void
editor_buffer_set_cursor_pos_relative(struct editor_buffer_t editor_buffer, int64_t relative_cursor)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_pos_relative_for_cursor_index(editor_buffer, i, relative_cursor);
    }

    sort_and_merge_cursors(editor_buffer);
}

void
editor_buffer_set_cursor_pos_relative_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t relative_cursor)
{
        struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

        cursor_info->char_pos += relative_cursor;

        if (cursor_info->char_pos < 0) {
            cursor_info->char_pos = 0;
        }

        int64_t total_length = rope_total_char_length(editor_buffer.current_screen->text);
        if (cursor_info->char_pos > total_length) {
            cursor_info->char_pos = total_length;
        }

        editor_screen_set_line_and_col_for_char_pos(cursor_info, editor_buffer.current_screen->text);
}

int64_t
editor_buffer_get_cursor_selection_start_row_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    return editor_buffer_get_virtual_cursor_row_for_point(editor_buffer,
                                                          cursor_info->selection_row,
                                                          cursor_info->selection_col,
                                                          virtual_line_length);
}

int64_t
editor_buffer_get_cursor_selection_start_col_virtual(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int64_t virtual_line_length)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    ensure_virtual_newline_length(editor_buffer.current_screen->lines, virtual_line_length);

    return editor_buffer_get_virtual_cursor_col_for_point(editor_buffer,
                                                          cursor_info->selection_row,
                                                          cursor_info->selection_col,
                                                          virtual_line_length);
}

int64_t
editor_buffer_cursor_is_selection(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    return cursor_info->is_selection;
}

void
editor_buffer_set_cursor_is_selection(struct editor_buffer_t editor_buffer, int8_t cursor_is_selection)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        editor_buffer_set_cursor_is_selection_for_cursor_index(editor_buffer, i, cursor_is_selection);
    }
}

void
editor_buffer_set_cursor_is_selection_for_cursor_index(struct editor_buffer_t editor_buffer, int64_t cursor_idx, int8_t cursor_is_selection)
{
    if (cursor_idx >= editor_buffer.current_screen->cursor_infos->length) { return; }

    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    if (!cursor_info->is_selection && cursor_is_selection) {
        cursor_info->selection_char_pos = cursor_info->char_pos;
        cursor_info->selection_row = cursor_info->row;
        cursor_info->selection_col = cursor_info->col;
    }

    cursor_info->is_selection = cursor_is_selection;
}

int64_t
editor_buffer_get_cursor_selection_start_row(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    return cursor_info->selection_row;
}

int64_t
editor_buffer_get_cursor_selection_start_col(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    return cursor_info->selection_col;
}

int64_t
editor_buffer_get_cursor_selection_start_pos(struct editor_buffer_t editor_buffer, int64_t cursor_idx)
{
    struct cursor_info_t *cursor_info = (struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, cursor_idx);

    return cursor_info->selection_char_pos;
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
editor_screen_set_line_and_col_for_char_pos(struct cursor_info_t *cursor_info, struct rope_t *text)
{
    int64_t row = rope_get_line_number_for_char_pos(text, cursor_info->char_pos);
    cursor_info->row = row;

    int64_t row_start = rope_char_number_at_line(text, row);
    int64_t col = cursor_info->char_pos - row_start;
    cursor_info->col = col;
}

void
kmp_prefix(const char *W, int64_t *T)
{
    int64_t pos = 1;
    int64_t cnd = 0;
    T[0] = -1;

    int64_t len_W = unicode_strlen(W);

    while (pos < len_W) {
        if (W[pos] == W[cnd]) {
            T[pos] = T[cnd];
            pos += 1;
            cnd += 1;
        } else {
            T[pos] = cnd;
            cnd = T[cnd];

            while (cnd >= 0 && W[pos] != W[cnd]) {
                cnd = T[cnd];
            }

            pos += 1;
            cnd += 1;
        }
    }

    T[pos] = cnd;
}

int64_t
kmp_search(struct editor_buffer_t editor_buffer, const char *W, int64_t start_char)
{
    // the beginning of the current match in S
    int64_t m = start_char;

    // the position of the current character in W
    int64_t i = 0;

    int64_t len_S = rope_total_byte_length(editor_buffer.current_screen->text);
    int64_t len_W = (int64_t) strlen(W);

    // the table
    int64_t *T = se_alloc(len_W, sizeof(int64_t));
    kmp_prefix(W, T);

    while (m + i < len_S) {
        char byte_at = rope_byte_at(editor_buffer.current_screen->text, m + i);

        if (byte_at == W[i]) {
            i += 1;
            if (i == len_W) {
                // occurrence found!
                free(T);
                return m;
            }
        } else if (T[i] > -1) {
            m = m + i - T[i];
            i = T[i];
        } else {
            m = m + i + 1;
            i = 0;
        }
    }

    free(T);
    return -1;
}

int64_t
kmp_search_backward(struct editor_buffer_t editor_buffer, const char *W, int64_t start_char)
{
    // the beginning of the current match in S
    int64_t m = start_char;

    int64_t len_W = (int64_t) strlen(W);

    // we need to reverse the string, since if you're searching backward character by character you are actually looking
    // for the reverse string :/
    char *WR = se_alloc(len_W, sizeof(char));
    for (int64_t i = 0; i < len_W; i++) {
        WR[i] = W[len_W - i - 1];
    }

    // the position of the current character in W
    int64_t i = 0;

    // the table
    int64_t T[len_W];
    kmp_prefix(WR, T);

    int64_t byte_at;

    while (m - i >= 0) {
        byte_at = rope_byte_at(editor_buffer.current_screen->text, m - i);

        if (byte_at == WR[i]) {
            i += 1;
            if (i == len_W) {
                // occurrence found!
                return m - len_W + 1;
            }
        } else {
            if (T[i] > -1) {
                m = m - i - T[i];
                i = T[i];
            } else {
                m = m - i - 1;
                i = 0;
            }
        }
    }

    free(WR);

    return -1;
}

int64_t
editor_buffer_search_forward(struct editor_buffer_t editor_buffer, const char *search, int64_t start_char)
{
    int64_t start_byte_offset = byte_for_char_at(editor_buffer.current_screen->text, start_char);
    int64_t byte_offset = kmp_search(editor_buffer, search, start_byte_offset);

    if (byte_offset == -1) {
        return -1;
    }
    return rope_char_for_byte_at(editor_buffer.current_screen->text, byte_offset);
}

int64_t
editor_buffer_search_backward(struct editor_buffer_t editor_buffer, const char *search, int64_t start_char)
{
    int64_t start_byte_offset = byte_for_char_at(editor_buffer.current_screen->text, start_char);
    int64_t byte_offset = kmp_search_backward(editor_buffer, search, start_byte_offset);
    if (byte_offset == -1) { return -1; }
    return rope_char_for_byte_at(editor_buffer.current_screen->text, byte_offset);
}

//int8_t
//is_word_breaking_char(char c)
//{
//    // todo(chad): make word boundaries configurable, the concept of 'word' should include more than just this stuff!!!
//    return c == ' ' || c == '\n' || c == '\t';
//}

void
editor_buffer_set_cursor_point_to_end_of_current_paragraph(struct editor_buffer_t editor_buffer, int64_t virtual, int64_t virtual_newline_length)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
        int64_t current_row = cursor.row;

        int64_t max_row;
        if (virtual) {
            max_row = editor_buffer_get_line_count_virtual(editor_buffer, virtual_newline_length);
        } else {
            max_row = editor_buffer_get_line_count(editor_buffer);
        }

        struct buf_t *current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                              current_row, 0,
                                                                              current_row + 1, 0);

        // advance to the start of a paragraph if we're in the middle
        while (current_row_buf->length <= 1 && current_row < max_row - 1) {
            current_row += 1;
            if (virtual) {
                current_row_buf = editor_buffer_get_text_between_points_virtual(editor_buffer,
                                                                                current_row, 0,
                                                                                current_row + 1, 0,
                                                                                virtual_newline_length);
            } else {
                current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                        current_row, 0,
                                                                        current_row + 1, 0);
            }
        }

        // advance to the end of it
        while (current_row_buf->length > 1 && current_row < max_row - 1) {
            current_row += 1;
            if (virtual) {
                current_row_buf = editor_buffer_get_text_between_points_virtual(editor_buffer,
                                                                                current_row, 0,
                                                                                current_row + 1, 0,
                                                                                virtual_newline_length);
            } else {
                current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                        current_row, 0,
                                                                        current_row + 1, 0);
            }
        }

        // set cursor to end of current line
        if (virtual) {
            int64_t line_length = editor_buffer_get_line_length_virtual(editor_buffer, current_row, virtual_newline_length);
            editor_buffer_set_cursor_point_virtual_for_cursor_index(editor_buffer, i, current_row, line_length, virtual_newline_length);
        } else {
            int64_t line_length = editor_buffer_get_line_length(editor_buffer, current_row);
            editor_buffer_set_cursor_point_for_cursor_index(editor_buffer, i, current_row, line_length);
        }
    }
}

void
editor_buffer_set_cursor_point_to_start_of_current_paragraph(struct editor_buffer_t editor_buffer, int64_t virtual, int64_t virtual_newline_length)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
        int64_t current_row = cursor.row;

        struct buf_t *current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                              current_row, 0,
                                                                              current_row + 1, 0);

        // advance to the start of a paragraph if we're in the middle
        while (current_row_buf->length <= 1 && current_row > 0) {
            current_row -= 1;
            if (virtual) {
                current_row_buf = editor_buffer_get_text_between_points_virtual(editor_buffer,
                                                                                current_row, 0,
                                                                                current_row + 1, 0,
                                                                                virtual_newline_length);
            } else {
                current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                        current_row, 0,
                                                                        current_row + 1, 0);
            }
        }

        // advance to the end of it
        while (current_row_buf->length > 1 && current_row > 0) {
            current_row -= 1;
            if (virtual) {
                current_row_buf = editor_buffer_get_text_between_points_virtual(editor_buffer,
                                                                                current_row, 0,
                                                                                current_row + 1, 0,
                                                                                virtual_newline_length);
            } else {
                current_row_buf = editor_buffer_get_text_between_points(editor_buffer,
                                                                        current_row, 0,
                                                                        current_row + 1, 0);
            }
        }

        // set cursor to end of current line
        if (virtual) {
            int64_t line_length = editor_buffer_get_line_length_virtual(editor_buffer, current_row, virtual_newline_length);
            editor_buffer_set_cursor_point_virtual_for_cursor_index(editor_buffer, i, current_row, line_length, virtual_newline_length);
        } else {
            int64_t line_length = editor_buffer_get_line_length(editor_buffer, current_row);
            editor_buffer_set_cursor_point_for_cursor_index(editor_buffer, i, current_row, line_length);
        }
    }
}

int8_t
str_contains(const char *chars, char c)
{
    int64_t char_pos = 0;
    char current_char = chars[char_pos];

    while (current_char != '\0') {
        if (current_char == c) { return 1; }

        char_pos += 1;
        current_char = chars[char_pos];
    }

    return 0;
}

void
editor_buffer_set_cursor_point_to_start_of_next_word(struct editor_buffer_t editor_buffer, const char *word_breaking_chars)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
        int64_t current_char = cursor.char_pos;
        int64_t max_char = editor_buffer_get_char_count(editor_buffer);

        int8_t skipping_word_breaking_chars = 0;
        const char *start_char = rope_char_at(editor_buffer.current_screen->text, current_char);
        if (current_char < max_char && str_contains(word_breaking_chars, *start_char)) {
            skipping_word_breaking_chars = 1;
        }

        int8_t found_start = 0;
        while (current_char < max_char) {
            const char *char_at = rope_char_at(editor_buffer.current_screen->text, current_char);

            if (char_at != NULL) {
                if (skipping_word_breaking_chars) {
                    // skip word breaking chars
                    while (char_at != NULL
                           && (str_contains(word_breaking_chars, *char_at))
                           && current_char < max_char) {
                        current_char += 1;
                        char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
                    }
                } else {
                    // skip non-spaces which are also non-word-breaking-chars
                    while (char_at != NULL
                           && (!str_contains(word_breaking_chars, *char_at) && !str_contains(" \n\t", *char_at))
                           && current_char < max_char) {
                        current_char += 1;
                        char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
                    }
                }

                // skip spaces
                while (char_at != NULL
                       && (str_contains(" \n\t", *char_at))
                       && current_char < max_char) {
                    current_char += 1;
                    char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
                }

                editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
                found_start = 1;
                break;
            }

            current_char += 1;
        }

        if (!found_start) {
            editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
        }
    }
}

void
editor_buffer_set_cursor_point_to_end_of_current_word(struct editor_buffer_t editor_buffer, const char *word_breaking_chars)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
        int64_t current_char = cursor.char_pos;
        int64_t max_char = editor_buffer_get_char_count(editor_buffer);

        const char *start_char = rope_char_at(editor_buffer.current_screen->text, current_char);

        // skip spaces
        while (start_char != NULL
               && (str_contains(" \n\t", *start_char))
               && current_char < max_char) {
            current_char += 1;
            start_char = rope_char_at(editor_buffer.current_screen->text, current_char);
        }

        int8_t skipping_word_breaking_chars = 0;
        start_char = rope_char_at(editor_buffer.current_screen->text, current_char);
        if (current_char < max_char && str_contains(word_breaking_chars, *start_char)) {
            skipping_word_breaking_chars = 1;
        }

        int8_t found_start = 0;
        while (current_char < max_char) {
            const char *char_at = rope_char_at(editor_buffer.current_screen->text, current_char);

            if (char_at != NULL) {
                if (skipping_word_breaking_chars) {
                    // skip word breaking chars
                    while (char_at != NULL
                           && (str_contains(word_breaking_chars, *char_at))
                           && current_char < max_char) {
                        current_char += 1;
                        char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
                    }
                } else {
                    // skip non-spaces which are also non-word-breaking-chars
                    while (char_at != NULL
                           && (!str_contains(word_breaking_chars, *char_at) && !str_contains(" \n\t", *char_at))
                           && current_char < max_char) {
                        current_char += 1;
                        char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
                    }
                }

                editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
                found_start = 1;
                break;
            }

            current_char += 1;
        }

        if (!found_start) {
            editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
        }
    }

//    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
//        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
//        int64_t current_char = cursor.char_pos + 1;
//        int64_t max_char = editor_buffer_get_char_count(editor_buffer);
//
//        int8_t skipping_word_breaking_chars = 0;
//        const char *start_char = rope_char_at(editor_buffer.current_screen->text, current_char);
//        if (current_char < max_char && str_contains(word_breaking_chars, *start_char)) {
//            skipping_word_breaking_chars = 1;
//        }
//
//        int8_t found_start = 0;
//        while (current_char < max_char) {
//            const char *char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
//
//            if (char_at != NULL) {
//                while (char_at != NULL
//                       && (skipping_word_breaking_chars != 0 == str_contains(word_breaking_chars, *char_at)
//                           && !str_contains(" \n\t", *char_at))
//                       && current_char < max_char) {
//                    current_char += 1;
//                    char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
//                }
//
//                editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
//                found_start = 1;
//                break;
//            }
//
//            current_char += 1;
//        }
//
//        if (!found_start) {
//            editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
//        }
//    }
}

void
editor_buffer_set_cursor_point_to_start_of_previous_word(struct editor_buffer_t editor_buffer, const char *word_breaking_chars)
{
    for (int64_t i = 0; i < editor_buffer.current_screen->cursor_infos->length; i++) {
        struct cursor_info_t cursor = *((struct cursor_info_t *) vector_at(editor_buffer.current_screen->cursor_infos, i));
        int64_t current_char = cursor.char_pos - 1;

        const char *char_at = rope_char_at(editor_buffer.current_screen->text, current_char);

        // skip all spaces
        while (str_contains(" \n\t", *char_at)) {
            current_char -= 1;
            char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
        }

        if (str_contains(word_breaking_chars, *char_at)) {
            // if we're at a word_breaking_char, then go until we hit a non-word-breaking-char
            while (str_contains(word_breaking_chars, *char_at) && current_char > 0) {
                current_char -= 1;
                char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
            }

            if (!str_contains(word_breaking_chars, *char_at)) {
                current_char += 1;
            }
        } else {
            // otherwise go until we hit a breaking char or a space
            while ((!str_contains(word_breaking_chars, *char_at) && !str_contains(" \n\t", *char_at))
                   && current_char > 0) {
                current_char -= 1;
                char_at = rope_char_at(editor_buffer.current_screen->text, current_char);
            }

            // !(A & B) <==> !A | !B
            if (str_contains(word_breaking_chars, *char_at) || str_contains(" \n\t", *char_at)) {
                current_char += 1;
            }
        }

        editor_buffer_set_cursor_pos_for_cursor_index(editor_buffer, i, current_char);
    }
}
