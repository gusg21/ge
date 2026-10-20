#include <ctype.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "cmds.h"
#include "defs.h"
#include "error_line.h"
#include "hilite.h"
#include "surface.h"

#define RGB_TO_1K(r,g,b) (r / 255.0) * 1000.0, (g / 255.0) * 1000.0, (b / 255.0) * 1000.0

/**
 * @brief Set the up terminal object
 * Example multiline comment
 */

void setup_terminal() {
    /* Setup reasonable terminal environment */
    initscr();            /* initialize the curses library */
    noecho();             /* Don't echo characters */
    cbreak();             /* Pass input immediately */
    keypad(stdscr, true); /* Enable all keys */
    nonl();               /* Suppress newline */
    raw();                /* Allow us to handle CTRL+c and CTRL+s, etc. */

    /* Initialize colors */
    start_color();
    for (int i = 0; i < 256; i++) {
        init_pair(i, i, COLOR_BLACK);
    }

    /* 0-1000 is the dumbest way to represent colors ever */
    init_color(SURFACE_BORDER_CIDX, 200, 200, 1000);
    init_color(SURFACE_TEXT_CIDX, RGB_TO_1K(189, 183, 107));
    init_color(SURFACE_LINE_NUMBER_CIDX, 500, 500, 500);
    init_color(SURFACE_KEYWORD_CIDX, RGB_TO_1K(9, 237, 249));
    init_color(SURFACE_COMMENT_CIDX, RGB_TO_1K(249, 9, 117));
    init_color(SURFACE_ERROR_CIDX, 1000, 200, 200);
    init_color(SURFACE_PUNCTUATION_CIDX, 1000, 1000, 1000);
    init_color(SURFACE_NUMBER_CIDX, RGB_TO_1K(117, 249, 9));
    init_color(SURFACE_OPERATOR_CIDX, RGB_TO_1K(237, 255, 47));
    init_color(SURFACE_DIRECTIVE_CIDX, RGB_TO_1K(105, 89, 237));
    init_color(SURFACE_STRING_CIDX, RGB_TO_1K(117, 249, 9));

    init_pair(SURFACE_TEXT_SELECTED_CIDX, COLOR_BLACK, SURFACE_TEXT_CIDX);
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

bool handle_key(struct ge_args args, int key) {
    if (isprint(key)) {
        insert_char(key);
    }

    else if (key == KEY_DOWN) {
        move_down();
        clear_selection();
    } else if (key == KEY_UP) {
        move_up();
        clear_selection();
    } else if (key == KEY_LEFT) {
        move_left();
        clear_selection();
    } else if (key == KEY_RIGHT) {
        move_right();
        clear_selection();
    }

    else if (key == KEY_SRIGHT) {
        try_start_selection();
        move_right();
        select_to_cursor();
    } else if (key == KEY_SLEFT) {
        try_start_selection();
        move_left();
        select_to_cursor();
    } else if (key == 0520) { /* shift+down */
        try_start_selection();
        move_down();
        select_to_cursor();
    } else if (key == 0521) { /* shift+up */
        try_start_selection();
        move_up();
        select_to_cursor();
    }

    else if (key == KEY_PPAGE) {
        page_up();
    } else if (key == KEY_NPAGE) {
        page_down();
    }

    else if (key == KEY_F(1)) {
        print_active_surface_info();
    }

    else if (key == 0177 || key == KEY_BACKSPACE) { /* backspace */
        backspace();
    }

    else if (key == CTRL('s')) {
        FILE* file = fopen(args.filename, "w");
        if (!file) {
            error_line_printf("Failed to write to %s!", args.filename);
        } else {
            buffer_to_file(&g_active_surf.buf, file);
            error_line_printf("Wrote to %s.", args.filename);
        }
        fclose(file);
    }

    else if (key == CTRL('c')) {
        endwin();
        refresh();
        return true;
    }

    return false;
}

int main(int argc, char* argv[]) {
    // FILE* file = fopen(".gitignore", "r");
    // buffer_from_file(&g_cur_surf.buf, file);

    setup_terminal();

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
        fclose(file);
    }

    for (;;) {
        /* Repaint */
        surface_repaint(&g_active_surf);
        error_line_repaint();

        /* Move cursor */
        int line, col;
        surface_get_cursor_display_pos(&g_active_surf, &line, &col);
        wmove(g_active_surf.win, line, col);

        /* Await keypress */
        int key = wgetch(g_active_surf.win); /* refresh, accept single keystroke of input */
        // error_line_printf("%o", c);

        /* Handle the keypress */
        bool wants_quit = handle_key(args, key);
        if (wants_quit) break;
    }

    return 0;
}