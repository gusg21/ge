#if !defined(CMDS_H)
#define CMDS_H

void move_up();
void move_down();
void move_left();
void move_right();
void page_up();
void page_down();
void replace_char(char c);
void insert_char(char c);
void backspace();
void try_start_selection();
void select_to_cursor();
void print_active_surface_info();
void clear_selection();
void insert_newline();

#endif  // CMDS_H
