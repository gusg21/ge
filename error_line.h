#if !defined(ERROR_LINE_H)
#define ERROR_LINE_H

#include <ncurses.h>

#define ERROR_LINE_MAX_LENGTH 1024
#define ERROR_LINE_INITIAL_TEXT "GE v" GE_VERSION

extern char g_error_line[ERROR_LINE_MAX_LENGTH];
extern WINDOW* g_error_line_win;

void error_line_init();
void error_line_repaint();
void error_line_printf(const char* fmt, ...);

#endif  // ERROR_LINE_H
