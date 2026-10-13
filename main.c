#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>

struct surface {
    char* buf;
};

static void finish() {
    refresh();
    endwin();

    exit(0);
}

static void setup() {
    initscr();      /* initialize the curses library */
    noecho();       /* Don't echo characters */
    cbreak();       /* Pass input immediately */
    keypad(stdscr, true); /* Enable all keys */

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

static void repaint_surface(struct surface* surf) {
    erase();

    move(5, 5);
    printw("%s", surf->buf);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, finish);
    setup();

    char* buf = calloc(1024, sizeof(char));

    struct surface surf = {0};
    surf.buf = "Hello world! This is GE.";

    repaint_surface(&surf);

    for (;;) {
        int c = getch(); /* refresh, accept single keystroke of input */
        
        repaint_surface(&surf);
    }

    return 0;
}