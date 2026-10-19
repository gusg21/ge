#if !defined(HILITE_H)
#define HILITE_H

#include <stddef.h>

#define HL_MAX_RANGES 2048

enum hl_kind {
    HL_KEYWORD,
    HL_COMMENT,
};

struct hl_range {
    int start_col;
    int end_col;
    int line;
    enum hl_kind kind;
};

struct hl_result {
    struct hl_range ranges[HL_MAX_RANGES];
    size_t count;
};

void hilite(struct hl_result* result, const char* text);

#endif // HILITE_H
