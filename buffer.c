#include "buffer.h"

#include <stdlib.h>
#include <string.h>

void buffer_init(struct buffer* buf) {
    buf->head = calloc(1, sizeof(struct buffer_node));
    buf->head->next = NULL;
}

int buffer_count_lines(struct buffer* buf) {
    struct buffer_node* node = buf->head;
    if (node == NULL) return 0;

    int count = 0;
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
    char c;

    char line[BUFFER_MAX_LINE_LENGTH] = { 0 };
    char* line_ptr = line;

    free(buf->head);
    buf->head = NULL;

    do {
        c = fgetc(file);

        if (c != EOF) {
            *line_ptr = c;
            line_ptr++;
        }

        if (c == '\n' || c == EOF) {
            struct buffer_node* new_line = buffer_add_line(buf);

            memcpy(new_line->text, line, BUFFER_MAX_LINE_LENGTH);
            line_ptr = line;
            memset(line, 0, BUFFER_MAX_LINE_LENGTH);
        }
    } while (c != EOF);

    /* Add last line */
    struct buffer_node* new_line = buffer_add_line(buf);
    memcpy(new_line->text, line, BUFFER_MAX_LINE_LENGTH);
}

void buffer_to_file(struct buffer* buf, FILE* file) {
    struct buffer_node* node = buf->head;
    while (node != NULL) {
        const char* c = node->text;
        while (*c != '\0') {
            fwrite(c, sizeof(char), 1, file);
            c++; /* Begone! */
        }

        node = node->next;
    }
}

/* This is going to be a gross expensive function, use sparingly */
char* buffer_to_string(struct buffer* buf) {
    struct buffer_node* node = buf->head;
    int line_ct = buffer_count_lines(buf);
    char* string = calloc(BUFFER_MAX_LINE_LENGTH * line_ct, sizeof(char)); /* Apparently allowed! Neat! */
    size_t string_size = 0;
    char* string_end = string;
    while (node != NULL) {
        size_t line_len = strnlen(node->text, BUFFER_MAX_LINE_LENGTH);
        string_size += line_len;
        memcpy(string_end, node->text, line_len);
        string_end += line_len;

        node = node->next;
    }

    return string;
}