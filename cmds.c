#include "cmds.h"

#include <string.h>

#include "surface.h"
#include "error_line.h"

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
    g_active_surf.cur.col--;
    surface_validate_line(&g_active_surf);
    surface_clamp_cursor_to_line(&g_active_surf);
}

void move_right() {
    g_active_surf.cur.col++;
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

/* Type a character into the current surface. */
void replace_char(char c) {
    buffer_set_char(&g_active_surf.buf, g_active_surf.cur.line, g_active_surf.cur.col, c);

    move_right();
}

void insert_char(char c) {
    char* current_line = buffer_get_line(&g_active_surf.buf, g_active_surf.cur.line);

    char* cursor = current_line + g_active_surf.cur.col;
    memcpy(cursor + 1, cursor, MAX_LINE_LENGTH - g_active_surf.cur.col - 1);
    *cursor = c;

    move_right();
}

void backspace() {
    if (g_active_surf.cur.col == 0) {
        return;
    }
    char* current_line = buffer_get_line(&g_active_surf.buf, g_active_surf.cur.line);

    char* cursor = current_line + g_active_surf.cur.col;
    memcpy(cursor - 1, cursor, MAX_LINE_LENGTH - g_active_surf.cur.col - 1);

    move_left();
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
