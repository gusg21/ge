#include "error_line.h"

#include <string.h>

#include "defs.h"

char g_error_line[ERROR_LINE_MAX_LENGTH];
WINDOW* g_error_line_win;

void error_line_init() {
    /* Clear + fill error line text */
    memset(g_error_line, 0, ERROR_LINE_MAX_LENGTH);
    memcpy(g_error_line, ERROR_LINE_INITIAL_TEXT, sizeof(ERROR_LINE_INITIAL_TEXT));

    /* Place window for error line */
    int maxx = getmaxx(stdscr);
    int maxy = getmaxy(stdscr);
    g_error_line_win = newwin(1, maxx, maxy - 1, 0);

    /* Draw box + text */
    error_line_repaint();
}

void error_line_repaint() {
    wattron(g_error_line_win, COLOR_PAIR(SURFACE_TEXT_CIDX));
    box(g_error_line_win, 0, 0);
    mvwprintw(g_error_line_win, 0, 0, "%s ", g_error_line);
    wrefresh(g_error_line_win);
}

/* Print some formatted text to the error line. */
void error_line_printf(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsprintf(g_error_line, fmt, args);
    va_end(args);
}