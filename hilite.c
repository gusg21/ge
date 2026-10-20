#include "hilite.h"

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

struct hl_str {
    char* name;
    size_t len;
};

static struct hl_str HL_KEYWORDS[] = {
    { "break", 5 },         { "case", 4 },     { "const", 5 },          { "constexpr", 9 }, { "continue", 8 },
    { "default", 7 },       { "do", 2 },       { "else", 4 },           { "enum", 4 },      { "extern", 6 },
    { "false", 5 },         { "for", 3 },      { "goto", 4 },           { "if", 2 },        { "inline", 6 },
    { "nullptr", 7 },       { "register", 8 }, { "restrict", 8 },       { "return", 6 },    { "signed", 6 },
    { "sizeof", 6 },        { "static", 6 },   { "static_assert", 13 }, { "struct", 6 },    { "switch", 6 },
    { "thread_local", 12 }, { "true", 4 },     { "typedef", 7 },        { "typeof", 6 },    { "typeof_unqual", 13 },
    { "union", 5 },         { "unsigned", 8 }, { "void", 4 },           { "volatile", 8 },  { "while", 5 }
};
static struct hl_str HL_DIRECTIVES[] = { { "#include", 8 }, { "#define", 7 } };
char HL_PUNCTUATIONS[] = { '{', '}', '[', ']', '(', ')' };
char HL_OPERATORS[] = { '+', '-', '*', '/', '=', '<', '>' };

#define ARR_LEN(x) sizeof(x) / sizeof(typeof(x[0]))

#define ADVANCE()    \
    if (c == '\n') { \
        line++;      \
        col = 0;     \
    } else {         \
        col++;       \
    }                \
    i++;             \
    ptr = &text[i];  \
    c = *ptr;

bool isident(char c) { return (isalpha(c) || c == '_'); }

void hilite(struct hl_result* result, const char* text) {
    int i = 0;
    int col = 0;
    int line = 0;
    const char* ptr = &text[i];
    char c = *ptr;
    while (c != '\0') {
		/* C Comment */
        if (strncmp(ptr, "/*", 2) == 0) {
            int start_col = col;
            int start_line = line;

            while (strncmp(ptr, "*/", 2) != 0) {
                ADVANCE();
            }

            result->ranges[result->count].start_col = start_col;
            result->ranges[result->count].end_col = col + 2;
            result->ranges[result->count].start_line = start_line;
            result->ranges[result->count].end_line = line;
            result->ranges[result->count].kind = HL_COMMENT;
            result->count++;

            /* Eat the comment closer */
            ADVANCE();
            ADVANCE();

            continue;
        }

        /* C++ Comment */
        if (strncmp(ptr, "//", 2) == 0) {
            int start_col = col;
            int start_line = line;

            while (c != '\n') {
                ADVANCE();
            }

            result->ranges[result->count].start_col = start_col;
            result->ranges[result->count].end_col = col;
            result->ranges[result->count].start_line = start_line;
            result->ranges[result->count].end_line = line;
            result->ranges[result->count].kind = HL_COMMENT;
            result->count++;

            continue;
        }

		/* Directive */
        bool found_dir = false;
        for (size_t dir_idx = 0; dir_idx < ARR_LEN(HL_DIRECTIVES); dir_idx++) {
            const char* dir = HL_DIRECTIVES[dir_idx].name;
            size_t dir_len = HL_DIRECTIVES[dir_idx].len;

            if (strncmp(ptr, dir, dir_len) == 0) {
                int start_col = col;

                while (c != '\n') {
                    ADVANCE();
                }

                result->ranges[result->count].start_col = start_col;
                result->ranges[result->count].end_col = col;
                result->ranges[result->count].start_line = line;
                result->ranges[result->count].end_line = line;
                result->ranges[result->count].kind = HL_DIRECTIVE;
                result->count++;

                found_dir = true;
                break;
            }
        }
        if (found_dir) continue;

		/* String */
		if (c == '"' || c == '\'') {
            int start_col = col;

			char delim = c;
			ADVANCE();
			while (c != delim) {
				ADVANCE();
			}
			ADVANCE();


			result->ranges[result->count].start_col = start_col;
            result->ranges[result->count].end_col = col;
            result->ranges[result->count].start_line = line;
            result->ranges[result->count].end_line = line;
            result->ranges[result->count].kind = HL_STRING;
            result->count++;

            continue;
		}

        /* Keyword */
        bool found_kw = false;
        for (size_t kw_idx = 0; kw_idx < ARR_LEN(HL_KEYWORDS); kw_idx++) {
            const char* keyword = HL_KEYWORDS[kw_idx].name;
            size_t kw_len = HL_KEYWORDS[kw_idx].len;

            if (strncmp(ptr, keyword, kw_len) == 0) {
                if (ptr[kw_len] == ' ') { /* Only match keywords followed by a space. */
                    result->ranges[result->count].start_col = col;
                    result->ranges[result->count].end_col = col + kw_len;
                    result->ranges[result->count].start_line = line;
                    result->ranges[result->count].end_line = line;
                    result->ranges[result->count].kind = HL_KEYWORD;
                    result->count++;

                    /* Eat the whole keyword */
                    for (int _ = 0; _ < kw_len; _++) { /* To consume the required trailing space */
                        ADVANCE();
                    }

                    found_kw = true;
                    break;
                }
            }
        }
        if (found_kw) continue;

        /* Punctuation */
        bool found_punc = false;
        for (size_t punc_i = 0; punc_i < ARR_LEN(HL_PUNCTUATIONS); punc_i++) {
            char punc = HL_PUNCTUATIONS[punc_i];
            if (c == punc) {
                int start_col = col;

                ADVANCE();

                result->ranges[result->count].start_col = start_col;
                result->ranges[result->count].end_col = col;
                result->ranges[result->count].start_line = line;
                result->ranges[result->count].end_line = line;
                result->ranges[result->count].kind = HL_PUNCTUATION;
                result->count++;

                found_punc = true;
                break;
            }
        }
        if (found_punc) continue;

        /* Operators */
        bool found_op = false;
        for (size_t op_i = 0; op_i < ARR_LEN(HL_OPERATORS); op_i++) {
            char op = HL_OPERATORS[op_i];
            if (c == op) {
                int start_col = col;

                ADVANCE();

                result->ranges[result->count].start_col = start_col;
                result->ranges[result->count].end_col = col;
                result->ranges[result->count].start_line = line;
                result->ranges[result->count].end_line = line;
                result->ranges[result->count].kind = HL_OPERATOR;
                result->count++;

                found_op = true;
                break;
            }
        }
        if (found_op) continue;

        /* Numbers */
        if (isdigit(c) && (i == 0 || !isident(text[i - 1]))) {
            int start_col = col;

            while (isdigit(c) || c == '.' || c == 'x' || c == 'b' || c == 'f') {
                ADVANCE();
            }

            result->ranges[result->count].start_col = start_col;
            result->ranges[result->count].end_col = col;
            result->ranges[result->count].start_line = line;
            result->ranges[result->count].end_line = line;
            result->ranges[result->count].kind = HL_NUMBER;
            result->count++;

            continue;
        }

        /* Advance a character */
        ADVANCE();
    }
}