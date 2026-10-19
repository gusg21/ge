#if !defined(ERROR_LINE_H)
#define ERROR_LINE_H

#include <ncurses.h>

#define MAX_ERROR_LINE_LENGTH 1024
#define INITIAL_ERROR_LINE_TEXT "GE v" GE_VERSION

extern char g_error_line[MAX_ERROR_LINE_LENGTH];
extern WINDOW* g_error_line_win;

void error_line_init();
void error_line_repaint();
void error_line_printf(const char* fmt, ...);

#endif  // ERROR_LINE_H
