#if !defined(SURFACE_H)
#define SURFACE_H

#include <ncurses.h>
#include <stdbool.h>

#include "buffer.h"
#include "hilite.h"

#define SURF_PAD 1

struct cursor {
    int col;
    int line;
};

struct selection {
    int start_col;
    int start_line;
    int end_col;
    int end_line;
    bool enabled;
};

struct surface {
    struct buffer buf;    /* The text contained in this surface */
    WINDOW* win;          /* The window that contains this surface */
    struct cursor cur;    /* The cursor position within the surface */
    int first_line;       /* Which line number is visible at the top of the surface (scroll) */
    struct selection sel; /* Selection info */
};

struct repaint_line_info {
    char* current_line;
    int screen_line_idx;
    int linenum_digit_ct;
};

extern struct surface g_active_surf;

void surface_init(struct surface* surf);
int surface_get_displayed_line_count(struct surface* surf);
int surface_get_line_number_digit_count(struct surface* surf);
void surface_repaint_line_simple(struct surface* surf, struct repaint_line_info info);
void surface_repaint_line_selection(struct surface* surf, struct repaint_line_info info);
int surface_get_hilite_color(enum hl_kind kind);
void surface_repaint(struct surface* surf);
void surface_validate_line(struct surface* surf);
void surface_get_cursor_display_pos(struct surface* surf, int* line, int* col);
void surface_clamp_cursor_to_line(struct surface* surf);
void surface_validate_selection(struct surface* surf);

#endif  // SURFACE_H
