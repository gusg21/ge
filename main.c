#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define STR(x) #x
#define XSTR(x) STR(x)
#define CTRL(x) ((x) & 0x1f)
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(value, min, max) MIN(max, MAX(min, value))

#define GE_VERSION "0.1"
#define MAX_LINE_LENGTH 1024
#define MAX_ERROR_LINE_LENGTH 1024
#define INITIAL_ERROR_LINE_TEXT "GE v" GE_VERSION
#define SURF_PAD 1

#define SURFACE_BORDER_COLOR_INDEX 1
#define SURFACE_TEXT_COLOR_INDEX 2

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

static struct surface g_active_surf = { 0 };
static char g_error_line[MAX_ERROR_LINE_LENGTH] = { 0 };
static WINDOW* g_error_line_win = NULL;

static void setup() {
    initscr();            /* initialize the curses library */
    noecho();             /* Don't echo characters */
    cbreak();             /* Pass input immediately */
    keypad(stdscr, true); /* Enable all keys */
    nonl();               /* Suppress newline */

    start_color();

    for (int i = 0; i < 256; i++) {
        init_pair(i, i, COLOR_BLACK);
    }

    init_color(SURFACE_BORDER_COLOR_INDEX, 200, 200, 1000);
    init_color(SURFACE_TEXT_COLOR_INDEX, 1000, 1000, 1000);
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
            int length = line_ptr - line;

            struct buffer_node* new_line = buffer_add_line(buf);

            memcpy(new_line->text, line, length);
            line_ptr = line;
            memset(line, 0, MAX_LINE_LENGTH);

            c = fgetc(file);
        } else {
            *line_ptr = c;
            line_ptr++;

            c = fgetc(file);
        }
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

int surface_get_line_count(struct surface* surf) { return getmaxy(surf->win) - SURF_PAD - SURF_PAD; }

void surface_repaint(struct surface* surf) {
    werase(surf->win);

    wattron(surf->win, COLOR_PAIR(1));
    box(surf->win, 0, 0);

    /* Render lines */
    int line_ct = surface_get_line_count(surf);
    int first_line = surf->first_line;
    for (int i = 0; i < line_ct; i++) {
        wattron(surf->win, COLOR_PAIR(2));
        mvwaddstr(surf->win, i + SURF_PAD, SURF_PAD, buffer_get_line(&surf->buf, first_line + i));
    }
    wrefresh(surf->win);
    mvwprintw(surf->win, 0, getmaxx(surf->win) - 10, "%d", surface_get_line_count(surf));
}

void surface_validate_state(struct surface* surf) {
    surf->cur.line = CLAMP(surf->cur.line, 0, buffer_count_lines(&surf->buf));
    surf->first_line = CLAMP(surf->first_line, 0, buffer_count_lines(&surf->buf));
}

void surface_get_cursor_display_pos(struct surface* surf, int* line, int* col) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), MAX_LINE_LENGTH);
    int clamped_col = CLAMP(surf->cur.col, 0, cur_line_len);

    *line = surf->cur.line - g_active_surf.first_line + SURF_PAD;
    *col = clamped_col + SURF_PAD;
}

void surface_clamp_cursor_to_line(struct surface* surf) {
    int cur_line_len = strnlen(buffer_get_line(&surf->buf, surf->cur.line), MAX_LINE_LENGTH);
    surf->cur.col = CLAMP(surf->cur.col, 0, cur_line_len);
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

void move_up() {
    if (g_active_surf.cur.line <= g_active_surf.first_line) {
        g_active_surf.first_line -= 1;
    }
    g_active_surf.cur.line--;
    surface_validate_state(&g_active_surf);
}

void move_down() {
    if (g_active_surf.cur.line + 1 >= g_active_surf.first_line + surface_get_line_count(&g_active_surf)) {
        g_active_surf.first_line += 1;
    }
    g_active_surf.cur.line++;
    surface_validate_state(&g_active_surf);
}

void move_left() {
    g_active_surf.cur.col--;
    surface_validate_state(&g_active_surf);
    surface_clamp_cursor_to_line(&g_active_surf);
}

void move_right() {
    g_active_surf.cur.col++;
    surface_validate_state(&g_active_surf);
    surface_clamp_cursor_to_line(&g_active_surf);
}

/* Type a character into the current surface. */
void replace_char(char c) {
    buffer_set_char(&g_active_surf.buf, g_active_surf.cur.line, g_active_surf.cur.col, c);
    
    move_right();
}

void insert_char(char c) {
    char* current_line = buffer_get_line(&g_active_surf.buf, g_active_surf.cur.line);

    char* cursor = current_line + g_active_surf.cur.col;
    memcpy(cursor + 1, cursor, MAX_LINE_LENGTH - g_active_surf.cur.col - 1);
    *cursor = c;

    move_right();
}

void backspace() {
    if (g_active_surf.cur.col == 0) {
        return;
    }
    char* current_line = buffer_get_line(&g_active_surf.buf, g_active_surf.cur.line);
    
    char* cursor = current_line + g_active_surf.cur.col;
    memcpy(cursor - 1, cursor, MAX_LINE_LENGTH - g_active_surf.cur.col - 1);
    
    move_left();
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
    // FILE* file = fopen(".gitignore", "r");
    // buffer_from_file(&g_cur_surf.buf, file);

    setup();

    /* Surface init */
    surface_init(&g_active_surf);

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
        buffer_from_file(&g_active_surf.buf, file);
    }

    for (;;) {
        surface_repaint(&g_active_surf);
        wrefresh(g_active_surf.win);

        error_line_repaint();

        int line, col;
        surface_get_cursor_display_pos(&g_active_surf, &line, &col);
        wmove(g_active_surf.win, line, col);

        int c = wgetch(g_active_surf.win); /* refresh, accept single keystroke of input */
        if (isprint(c)) {
            insert_char(c);
        } else if (c == KEY_DOWN) {
            move_down();
        } else if (c == KEY_UP) {
            move_up();
        } else if (c == KEY_LEFT) {
            move_left();
        } else if (c == KEY_RIGHT) {
            move_right();
        } else if (c == 0177) {
            backspace();
        }
        error_line_printf("%o", c);
    }

    return 0;
}