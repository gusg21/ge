#if !defined(BUFFER_H)
#define BUFFER_H

#include <stdio.h>

#define MAX_LINE_LENGTH 1024

struct buffer_node {
    char text[MAX_LINE_LENGTH];
    struct buffer_node* next;
};

struct buffer {
    struct buffer_node* head;
};

void buffer_init(struct buffer* buf);
int buffer_count_lines(struct buffer* buf);
char* buffer_get_line(struct buffer* buf, int line);
void buffer_set_char(struct buffer* buf, int line, int col, char c);
struct buffer_node* buffer_find_tail(struct buffer* buf);
struct buffer_node* buffer_add_line(struct buffer* buf);
void buffer_from_memory(struct buffer* buf, const char* text);
void buffer_from_file(struct buffer* buf, FILE* file);
void buffer_to_file(struct buffer* buf, FILE* file);
char* buffer_to_string(struct buffer* buf);

#endif // BUFFER_H
