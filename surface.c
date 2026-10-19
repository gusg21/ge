#include "surface.h"

#include <string.h>
#include <math.h>

#include "defs.h"
#include "error_line.h"

struct surface g_active_surf;

void surface_init(struct surface* surf) {
    /* Make window */
    int maxy = getmaxy(stdscr);
    int maxx = getmaxx(stdscr);
    surf->win = newwin(maxy - 1, maxx, 0, 0);
    keypad(surf->win, true);

    /* Make buffer */
    buffer_init(&surf->buf);

    /* Init cursor */
    surf->cur.col = 0;
    surf->cur.line = 0;

    surf->first_line = 0;
    surf->sel.enabled = false;
}

int surface_get_displayed_line_count(struct surface* surf) { return getmaxy(surf->win) - SURF_PAD - SURF_PAD; }

int surface_get_line_number_digit_count(struct surface* surf) {
    int line_ct = buffer_count_lines(&surf->buf);
    return log10l(line_ct) + 1;
}

void surface_repaint_line_simple(struct surface* surf, struct repaint_line_info info) {
    int line_idx = surf->first_line + info.screen_line_idx;

    /* Line number */
    wattron(surf->win, COLOR_PAIR(SURFACE_LINE_NUMBER_CIDX));
    mvwprintw(surf->win, info.screen_line_idx + SURF_PAD, SURF_PAD, "%.*d", info.linenum_digit_ct, line_idx + 1);

    /* Line text */
    wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_CIDX));
    mvwaddstr(surf->win, info.screen_line_idx + SURF_PAD, SURF_PAD + info.linenum_digit_ct + 1, info.current_line);
}

void surface_repaint_line_selection(struct surface* surf, struct repaint_line_info info) {
    int line_idx = surf->first_line + info.screen_line_idx;
    int start_line = MIN(surf->sel.start_line, surf->sel.end_line);
    int end_line = MAX(surf->sel.start_line, surf->sel.end_line);

    if (line_idx == start_line) {
        int start_col, end_col;
        if (line_idx == end_line) {
            /* Single-line selection */
            start_col = MIN(surf->sel.start_col, surf->sel.end_col);
            end_col = MAX(surf->sel.start_col, surf->sel.end_col);
        } else {
            /* Beginning of multi-line selection */
            start_col = surf->sel.start_col;
            if (surf->sel.start_line > surf->sel.end_line) {
                start_col = surf->sel.end_col;
            }
            end_col = MAX_LINE_LENGTH;
        }

        char substr[MAX_LINE_LENGTH] = { 0 };
        int selection_length = end_col - start_col;
        memcpy(substr, info.current_line + start_col, selection_length);

        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURF_PAD, SURF_PAD + start_col + info.linenum_digit_ct + 1, substr);
    }

    else if (line_idx > start_line && line_idx < end_line) {
        /* Whole line selected (in the middle of a selection) */
        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURF_PAD, SURF_PAD + info.linenum_digit_ct + 1, info.current_line);
    }

    else if (line_idx == end_line) {
        /* End of a multi-line selection */
        int end_col = surf->sel.end_col;
        if (surf->sel.start_line > surf->sel.end_line) {
            end_col = surf->sel.start_col;
        }

        char substr[MAX_LINE_LENGTH] = { 0 };
        int selection_length = end_col;
        memcpy(substr, info.current_line, selection_length);

        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURF_PAD, SURF_PAD + info.linenum_digit_ct + 1, substr);
    }
}

int surface_get_hilite_color(enum hl_kind kind) {
    switch (kind) {
        case HL_KEYWORD:
            return SURFACE_KEYWORD_CIDX;

        case HL_COMMENT:
            return SURFACE_COMMENT_CIDX;

        default:
            return SURFACE_ERROR_CIDX;
    }
}

void surface_repaint(struct surface* surf) {
    werase(surf->win);

    wattron(surf->win, COLOR_PAIR(SURFACE_BORDER_CIDX));
    box(surf->win, 0, 0);

    /* Highlighting */
    char* surface_text = buffer_to_string(&surf->buf);
    struct hl_result result = { 0 };
    hilite(&result, surface_text);

    /* Render lines */
    int line_ct = surface_get_displayed_line_count(surf);
    int total_line_ct = buffer_count_lines(&surf->buf);
    int first_line = surf->first_line;
    int linenum_digit_ct = surface_get_line_number_digit_count(surf);
    for (int i = 0; i < line_ct; i++) {
        int line_idx = first_line + i;
        char* current_line = buffer_get_line(&surf->buf, line_idx);

        if (line_idx >= total_line_ct) {
            break;
        }

        struct repaint_line_info info =
            (struct repaint_line_info){ /* Information to be passed to the repainting sub-functions */
                                        .current_line = current_line,
                                        .linenum_digit_ct = linenum_digit_ct,
                                        .screen_line_idx = i
            };

        /* Draw unformatted line */
        surface_repaint_line_simple(surf, info);

        /* Draw highlighting */
        for (int range_i = 0; range_i < result.count; range_i++) {
            struct hl_range* range = &result.ranges[range_i];

            char line[MAX_LINE_LENGTH] = { 0 };
            wattron(surf->win, COLOR_PAIR(surface_get_hilite_color(range->kind)));
            memcpy(line, current_line + range->start_col, range->end_col - range->start_col);
            mvwaddstr(surf->win, info.screen_line_idx + SURF_PAD,
                      SURF_PAD + info.linenum_digit_ct + 1 + range->start_col, line);
        }
        error_line_printf("%d", result.count);

        /* Draw selection for this line */
        if (surf->sel.enabled) {
            surface_repaint_line_selection(surf, info);
        }
    }

    wrefresh(surf->win);
}

void surface_validate_line(struct surface* surf) {
    surf->cur.line = CLAMP(surf->cur.line, 0, buffer_count_lines(&surf->buf) - 1);
    surf->first_line = CLAMP(surf->first_line, 0, buffer_count_lines(&surf->buf));
}

void surface_get_cursor_display_pos(struct surface* surf, int* line, int* col) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), MAX_LINE_LENGTH);
    int clamped_col = CLAMP(surf->cur.col, 0, cur_line_len);
    int linenum_digit_count = surface_get_line_number_digit_count(surf);

    *line = surf->cur.line - g_active_surf.first_line + SURF_PAD;
    *col = clamped_col + SURF_PAD + linenum_digit_count + 1;
}

void surface_clamp_cursor_to_line(struct surface* surf) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), MAX_LINE_LENGTH);
    surf->cur.col = CLAMP(surf->cur.col, 0, cur_line_len);
}

void surface_validate_selection(struct surface* surf) {
    struct buffer* buf = &surf->buf;
    int line_ct = buffer_count_lines(buf);
    surf->sel.start_line = CLAMP(surf->sel.start_line, 0, line_ct);
    surf->sel.end_line = CLAMP(surf->sel.end_line, 0, line_ct);

    /* We are intentionally not clamping the selection to the line lengths to allow selections traveling over shorter
     * lines to be unaffected */
    surf->sel.start_col = MAX(surf->sel.start_col, 0);
    surf->sel.end_col = MAX(surf->sel.end_col, 0);
}