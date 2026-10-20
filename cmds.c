#include "cmds.h"

#include <stdlib.h>
#include <string.h>

#include "error_line.h"
#include "surface.h"

void move_up() {
    g_active_surf.cur.line--;
    if (g_active_surf.cur.line < g_active_surf.first_line) {
        g_active_surf.first_line -= 1;
    }
    surface_validate_line(&g_active_surf);
}

void move_down() {
    g_active_surf.cur.line++;
    if (g_active_surf.cur.line >= g_active_surf.first_line + surface_get_displayed_line_count(&g_active_surf)) {
        g_active_surf.first_line += 1;
    }
    surface_validate_line(&g_active_surf);
}

void move_left() {
    surface_clamp_cursor_to_line(&g_active_surf);
    g_active_surf.cur.col--;

    if (g_active_surf.cur.col < 0 && g_active_surf.cur.line > 0) {
        g_active_surf.cur.line--;
        int line_len = surface_get_current_line_length(&g_active_surf);
        g_active_surf.cur.col = line_len - 1; /* Ignore the new line */
    }

    surface_validate_line(&g_active_surf);
    surface_clamp_cursor_to_line(&g_active_surf);
}

void move_right() {
    surface_clamp_cursor_to_line(&g_active_surf);
    g_active_surf.cur.col++;

    int line_len = surface_get_current_line_length(&g_active_surf);
    int line_ct = buffer_count_lines(&g_active_surf.buf);
    if (g_active_surf.cur.col > line_len - 1 && g_active_surf.cur.line < line_ct - 1) {
        g_active_surf.cur.col = 0;
        g_active_surf.cur.line++;
    }

    surface_validate_line(&g_active_surf);
    surface_clamp_cursor_to_line(&g_active_surf);
}

void page_up() {
    int page_height = surface_get_displayed_line_count(&g_active_surf);

    g_active_surf.cur.line -= page_height;
    g_active_surf.first_line = g_active_surf.cur.line - page_height / 2;

    surface_validate_line(&g_active_surf);
}

void page_down() {
    int page_height = surface_get_displayed_line_count(&g_active_surf);

    g_active_surf.cur.line += page_height;
    g_active_surf.first_line = g_active_surf.cur.line - page_height / 2;

    surface_validate_line(&g_active_surf);
}

void replace_char(char c) {
    buffer_set_char(&g_active_surf.buf, g_active_surf.cur.line, g_active_surf.cur.col, c);

    move_right();
}

/* Type a character into the current surface. */
void insert_char(char c) {
    char* current_line = buffer_get_line(&g_active_surf.buf, g_active_surf.cur.line);

    char* cursor = current_line + g_active_surf.cur.col;
    memcpy(cursor + 1, cursor, BUFFER_MAX_LINE_LENGTH - g_active_surf.cur.col - 1);
    *cursor = c;

    move_right();
}

void backspace() {
    struct buffer_node* cur_line_node = buffer_get_node(&g_active_surf.buf, g_active_surf.cur.line);
    char* current_line = cur_line_node->text;

    if (g_active_surf.cur.col == 0) {
        struct buffer_node* last_node = buffer_get_node(&g_active_surf.buf, g_active_surf.cur.line - 1);
        last_node->next = cur_line_node->next; /* Remove the current line from the list */

        int last_line_len = strnlen(last_node->text, BUFFER_MAX_LINE_LENGTH);
        int cur_line_len = strnlen(current_line, BUFFER_MAX_LINE_LENGTH);
        memcpy(last_node->text + last_line_len - 1, cur_line_node->text, cur_line_len);

        free(cur_line_node);

        g_active_surf.cur.line--;
        g_active_surf.cur.col = last_line_len - 1;
    }
    else {
        cur_line_node = buffer_get_node(&g_active_surf.buf, g_active_surf.cur.line);
        current_line = cur_line_node->text;
        char* cursor = current_line + g_active_surf.cur.col;
        memcpy(cursor - 1, cursor, BUFFER_MAX_LINE_LENGTH - g_active_surf.cur.col - 1);

        g_active_surf.cur.col--;
    }

    surface_clamp_cursor_to_line(&g_active_surf);
}

void try_start_selection() {
    if (!g_active_surf.sel.enabled) {
        g_active_surf.sel.start_line = g_active_surf.cur.line;
        g_active_surf.sel.start_col = g_active_surf.cur.col;
    }

    g_active_surf.sel.enabled = true;
}

void select_to_cursor() {
    g_active_surf.sel.end_col = g_active_surf.cur.col;
    g_active_surf.sel.end_line = g_active_surf.cur.line;
    surface_validate_selection(&g_active_surf);
}

void print_active_surface_info() { error_line_printf("%d lines", buffer_count_lines(&g_active_surf.buf)); }

void clear_selection() { g_active_surf.sel.enabled = false; }

void insert_newline() {
    struct buffer_node* cur_line_node = buffer_get_node(&g_active_surf.buf, g_active_surf.cur.line);
    struct buffer_node* next_node = cur_line_node->next;
    struct buffer_node* new_line_node = calloc(1, sizeof(struct buffer_node));

    // new_line_node->text[0] = '\n';
    int line_len = strnlen(cur_line_node->text, BUFFER_MAX_LINE_LENGTH);
    int new_line_len = line_len - g_active_surf.cur.col;
    char* cursor = cur_line_node->text + g_active_surf.cur.col;
    memcpy(new_line_node->text, cursor, new_line_len);
    memset(cursor, 0, new_line_len);
    *cursor = '\n';

    /* Insert new line node into list */
    cur_line_node->next = new_line_node;
    new_line_node->next = next_node;

    g_active_surf.cur.line++;
    g_active_surf.cur.col = 0;
}