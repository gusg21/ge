#if !defined(CIDX_H)
#define CIDX_H

#define STR(x) #x
#define XSTR(x) STR(x)
#define CTRL(x) ((x) & 0x1f)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, min, max) MIN(max, MAX(min, value))

#define GE_VERSION "0.1"

#define SURFACE_BORDER_CIDX 1
#define SURFACE_TEXT_CIDX 2
#define SURFACE_TEXT_SELECTED_CIDX 3
#define SURFACE_LINE_NUMBER_CIDX 4
#define SURFACE_KEYWORD_CIDX 5
#define SURFACE_COMMENT_CIDX 6
#define SURFACE_ERROR_CIDX 7

#endif  // CIDX_H
