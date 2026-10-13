#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define STR(x) #x
#define XSTR(x) STR(x)
#define CTRL(x) ((x) & 0x1f)

#define GE_VERSION "0.1"
#define MAX_LINE_LENGTH 1024
#define MAX_ERROR_LINE_LENGTH 1024
#define INITIAL_ERROR_LINE_TEXT "GE v" GE_VERSION
#define SURF_PAD 1

struct buffer_node {
    char text[MAX_LINE_LENGTH];
    struct buffer_node* next;
};

struct buffer {
    struct buffer_node* head;
};

struct cursor {
    int col;
    int line;
};

struct surface {
    struct buffer buf; /* The text contained in this surface */
    WINDOW* win;       /* The window that contains this surface */
    struct cursor cur; /* The cursor position within the surface */
    int first_line;    /* Which line number is visible at the top of the surface (scroll) */
};

static struct surface g_cur_surf = { 0 };
static char g_error_line[MAX_ERROR_LINE_LENGTH] = { 0 };
static WINDOW* g_error_line_win = NULL;

static void setup() {
    initscr();            /* initialize the curses library */
    noecho();             /* Don't echo characters */
    cbreak();             /* Pass input immediately */
    keypad(stdscr, true); /* Enable all keys */
    nonl();               /* Suppress newline */

    if (has_colors()) {
        start_color();

        init_pair(1, COLOR_RED, COLOR_BLACK);
        init_pair(2, COLOR_GREEN, COLOR_BLACK);
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);
        init_pair(4, COLOR_BLUE, COLOR_BLACK);
        init_pair(5, COLOR_CYAN, COLOR_BLACK);
        init_pair(6, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(7, COLOR_WHITE, COLOR_BLACK);
    }
}

void buffer_init(struct buffer* buf) {
    buf->head = calloc(1, sizeof(struct buffer_node));
    buf->head->next = NULL;
}

int buffer_count_lines(struct buffer* buf) {
    struct buffer_node* node = buf->head;
    int count = 1;
    while (node->next) {
        count++;
        node = node->next;
    }
    return count;
}

/* Line is 0-indexed */
char* buffer_get_line(struct buffer* buf, int line) {
    struct buffer_node* node = buf->head;
    int cur_line = 0;
    do {
        if (cur_line == line) {
            return node->text;
        }
        node = node->next;
        cur_line++;
    } while (node->next);

    return NULL;  // Not enough lines.
}

void buffer_set_char(struct buffer* buf, int line, int col, char c) {
    char* line_buf = buffer_get_line(buf, line);
    if (line_buf) {
        line_buf[col] = c;
    }
}

struct buffer_node* buffer_find_tail(struct buffer* buf) {
    struct buffer_node* node = buf->head;
    if (node == NULL) {
        return NULL;
    }

    while (node->next) {
        node = node->next;
    }
    return node;
}

struct buffer_node* buffer_add_line(struct buffer* buf) {
    struct buffer_node* tail = buffer_find_tail(buf);
    if (tail == NULL) {
        /* Buffer is empty, add a first line. */
        buf->head = calloc(1, sizeof(struct buffer_node));
        return buf->head;
    }

    tail->next = calloc(1, sizeof(struct buffer_node));
    tail->next->next = NULL;
    return tail->next;
}

void buffer_from_memory(struct buffer* buf, const char* text) {
    const char* pointer = text;
    const char* line_start = text;
    while (*pointer != '\0') {
        if (*pointer == '\n') {
            size_t length = pointer - line_start;

            struct buffer_node* new_line = buffer_add_line(buf);
            memcpy(new_line->text, line_start, length);

            line_start = pointer + 1;
        }

        pointer++;
    }
}

void buffer_from_file(struct buffer* buf, FILE* file) {
    char c = fgetc(file);

    char line[MAX_LINE_LENGTH] = { 0 };
    char* line_ptr = line;

    free(buf->head);
    buf->head = NULL;

    while (c != EOF) {
        if (c == '\n') {
            size_t length = line_ptr - line;

            struct buffer_node* new_line = buffer_add_line(buf);

            memcpy(new_line->text, line, length);

            memset(line, 0, MAX_LINE_LENGTH);
            line_ptr = line;

            /* Eat the new line */
            while (c == '\n') {
                c = fgetc(file);
                if (c == '\n') {
                    buffer_add_line(buf);
                }
            }
        }

        *line_ptr = c;
        line_ptr++;
        c = fgetc(file);
    }
}

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
}

void surface_repaint(struct surface* surf) {
    werase(surf->win);
    box(surf->win, 0, 0);

    /* Render lines */
    int line_ct = getmaxy(surf->win) - SURF_PAD - SURF_PAD;
    int first_line = surf->first_line;
    for (int i = 0; i < line_ct; i++) {
        mvwaddstr(surf->win, i + SURF_PAD, SURF_PAD, buffer_get_line(&surf->buf, first_line + i));
    }
    wrefresh(surf->win);
}

void error_line_init() {
    memcpy(g_error_line, INITIAL_ERROR_LINE_TEXT, sizeof(INITIAL_ERROR_LINE_TEXT));

    int maxx = getmaxx(stdscr);
    int maxy = getmaxy(stdscr);
    g_error_line_win = newwin(1, maxx, maxy - 1, 0);

    box(g_error_line_win, 0, 0);
    mvwprintw(g_error_line_win, 0, 0, "%s ", g_error_line);
}

void error_line_repaint() {
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

/* Type a character into the current surface. */
void type_char(char c) {
    buffer_set_char(&g_cur_surf.buf, g_cur_surf.cur.line, g_cur_surf.cur.col, c);
    g_cur_surf.cur.col++;
}

struct ge_args {
    const char* filename;
};

struct ge_args parse_args(int argc, char* argv[]) {
    struct ge_args args = { 0 };

    /* Default arguments */
    args.filename = "main.c";

    /* Parse arguments */
    for (int i = 1; i < argc; i++) {
        args.filename = argv[i];
    }

    return args;
}

int main(int argc, char* argv[]) {
    setup();

    /* Surface init */
    surface_init(&g_cur_surf);

    /* Error line init */
    error_line_init();

    /* Parse args */
    struct ge_args args = parse_args(argc, argv);

    /* Load file */
    FILE* file = fopen(args.filename, "r");
    if (file == NULL) {
        error_line_printf("Failed to read from %s.", args.filename);
    } else {
        error_line_printf("Read from %s.", args.filename);
        buffer_from_file(&g_cur_surf.buf, file);
    }

    for (;;) {
        surface_repaint(&g_cur_surf);
        wrefresh(g_cur_surf.win);

        error_line_repaint();

        int padding = 1;
        wmove(g_cur_surf.win, g_cur_surf.cur.line + padding, g_cur_surf.cur.col + padding);

        char c = wgetch(g_cur_surf.win); /* refresh, accept single keystroke of input */
        if (isprint(c)) {
            type_char(c);
        } else if (KEY_DOWN) {
            g_cur_surf.first_line++;
            error_line_printf("%d", g_cur_surf.first_line);
        }
    }

    return 0;
}