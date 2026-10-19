#include "hilite.h"

#include <string.h>
#include <stdbool.h>

static struct {
    char* name;
    size_t len;
} HL_KEYWORDS[] = {
    { "break", 5 },         { "case", 4 },     { "const", 5 },          { "constexpr", 9 }, { "continue", 8 },
    { "default", 7 },       { "do", 2 },       { "else", 4 },           { "enum", 4 },      { "extern", 6 },
    { "false", 5 },         { "for", 3 },      { "goto", 4 },           { "if", 2 },        { "inline", 6 },
    { "nullptr", 7 },       { "register", 8 }, { "restrict", 8 },       { "return", 6 },    { "signed", 6 },
    { "sizeof", 6 },        { "static", 6 },   { "static_assert", 13 }, { "struct", 6 },    { "switch", 6 },
    { "thread_local", 12 }, { "true", 4 },     { "typedef", 7 },        { "typeof", 6 },    { "typeof_unqual", 13 },
    { "union", 5 },         { "unsigned", 8 }, { "void", 4 },           { "volatile", 8 },  { "while", 5 }
};
#define ARR_LEN(x) sizeof(HL_KEYWORDS) / sizeof(typeof(HL_KEYWORDS[0]))

void hilite(struct hl_result* result, const char* text) {
    int i = 0;
    int col = 0;
    int line = 0;
    while (text[i] != '\0') {
        const char* ptr = &text[col];
        char c = text[i];
        
        /* Keyword */
        bool found_kw = false;
        for (size_t kw_idx = 0; kw_idx < ARR_LEN(HL_KEYWORDS); kw_idx++) {
            const char* keyword = HL_KEYWORDS[kw_idx].name;
            size_t kw_len = HL_KEYWORDS[kw_idx].len;

            if (strncmp(ptr, keyword, kw_len) == 0) {
                result->ranges[result->count].start_col = col;
                result->ranges[result->count].end_col = col + kw_len;
                result->ranges[result->count].kind = HL_KEYWORD;
                result->ranges[result->count].line = line;
                result->count++;

                col += kw_len;
                found_kw = true;
            }
        }
        if (found_kw) {
            continue;
        }

        /* Advance a character */
        i++;
        col++;
        if (text[col] == '\n') {
            line++;
            col = 0;
        }
    }
}