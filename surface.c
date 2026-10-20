#include "surface.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

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

int surface_get_displayed_line_count(struct surface* surf) { return getmaxy(surf->win) - SURFACE_PAD - SURFACE_PAD; }

int surface_get_line_number_digit_count(struct surface* surf) {
    int line_ct = buffer_count_lines(&surf->buf);
    return log10l(line_ct) + 1;
}

void surface_repaint_line_simple(struct surface* surf, struct repaint_line_info info) {
    int line_idx = surf->first_line + info.screen_line_idx;

    /* Line number */
    wattron(surf->win, COLOR_PAIR(SURFACE_LINE_NUMBER_CIDX));
    mvwprintw(surf->win, info.screen_line_idx + SURFACE_PAD, SURFACE_PAD, "%.*d", info.linenum_digit_ct, line_idx + 1);

    /* Line text */
    wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_CIDX));
    mvwaddstr(surf->win, info.screen_line_idx + SURFACE_PAD, SURFACE_PAD + info.linenum_digit_ct + 1,
              info.current_line);
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
            end_col = BUFFER_MAX_LINE_LENGTH;
        }

        char substr[BUFFER_MAX_LINE_LENGTH] = { 0 };
        int selection_length = end_col - start_col;
        memcpy(substr, info.current_line + start_col, selection_length);

        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURFACE_PAD, SURFACE_PAD + start_col + info.linenum_digit_ct + 1,
                  substr);
    }

    else if (line_idx > start_line && line_idx < end_line) {
        /* Whole line selected (in the middle of a selection) */
        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURFACE_PAD, SURFACE_PAD + info.linenum_digit_ct + 1,
                  info.current_line);
    }

    else if (line_idx == end_line) {
        /* End of a multi-line selection */
        int end_col = surf->sel.end_col;
        if (surf->sel.start_line > surf->sel.end_line) {
            end_col = surf->sel.start_col;
        }

        char substr[BUFFER_MAX_LINE_LENGTH] = { 0 };
        int selection_length = end_col;
        memcpy(substr, info.current_line, selection_length);

        wattron(surf->win, COLOR_PAIR(SURFACE_TEXT_SELECTED_CIDX));
        mvwaddstr(surf->win, info.screen_line_idx + SURFACE_PAD, SURFACE_PAD + info.linenum_digit_ct + 1, substr);
    }
}

int surface_get_hilite_color(enum hl_kind kind) {
    switch (kind) {
        case HL_KEYWORD:
            return SURFACE_KEYWORD_CIDX;

        case HL_COMMENT:
            return SURFACE_COMMENT_CIDX;

        case HL_PUNCTUATION:
            return SURFACE_PUNCTUATION_CIDX;

        case HL_NUMBER:
            return SURFACE_NUMBER_CIDX;

        case HL_OPERATOR:
            return SURFACE_OPERATOR_CIDX;

        case HL_DIRECTIVE:
            return SURFACE_DIRECTIVE_CIDX;

        case HL_STRING:
            return SURFACE_STRING_CIDX;

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
    struct hl_result* hl_result = calloc(1, sizeof(struct hl_result));
    hilite(hl_result, surface_text);

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
        for (int range_i = 0; range_i < hl_result->count; range_i++) {
            struct hl_range* range = &hl_result->ranges[range_i];

            int start_col = 0;
            int end_col = 0;

            if (range->start_line == line_idx && range->end_line == line_idx) {
                start_col = range->start_col;
                end_col = range->end_col;
            }
            if (line_idx > range->start_line && line_idx < range->end_line) {
                start_col = 0;
                end_col = BUFFER_MAX_LINE_LENGTH;
            }
            if (line_idx == range->start_line && line_idx < range->end_line) {
                start_col = range->start_col;
                end_col = BUFFER_MAX_LINE_LENGTH;
            }
            if (line_idx > range->start_line && line_idx == range->end_line) {
                start_col = 0;
                end_col = range->end_col;
            }

            char line[BUFFER_MAX_LINE_LENGTH] = { 0 };
            wattron(surf->win, COLOR_PAIR(surface_get_hilite_color(range->kind)));
            memcpy(line, current_line + start_col, end_col - start_col);
            mvwaddstr(surf->win, info.screen_line_idx + SURFACE_PAD,
                      SURFACE_PAD + info.linenum_digit_ct + 1 + start_col, line);
        }
        error_line_printf("%d:%d", surf->cur.line, surf->cur.col);

        /* Draw selection for this line */
        if (surf->sel.enabled) {
            surface_repaint_line_selection(surf, info);
        }
    }

    wrefresh(surf->win);
    free(hl_result);
    free(surface_text);
}

void surface_validate_line(struct surface* surf) {
    surf->cur.line = CLAMP(surf->cur.line, 0, buffer_count_lines(&surf->buf) - 1);
    surf->first_line = CLAMP(surf->first_line, 0, buffer_count_lines(&surf->buf));
}

void surface_get_cursor_display_pos(struct surface* surf, int* line, int* col) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), BUFFER_MAX_LINE_LENGTH);
    int clamped_col = CLAMP(surf->cur.col, 0, cur_line_len - 1); /* -1 to accommodate the newline */
    int linenum_digit_count = surface_get_line_number_digit_count(surf);

    *line = surf->cur.line - g_active_surf.first_line + SURFACE_PAD;
    *col = clamped_col + SURFACE_PAD + linenum_digit_count + 1;
}

void surface_clamp_cursor_to_line(struct surface* surf) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), BUFFER_MAX_LINE_LENGTH);
    surf->cur.col = CLAMP(surf->cur.col, 0, cur_line_len - 1);
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