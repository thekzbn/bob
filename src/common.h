#ifndef BOB_COMMON_H
#define BOB_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

static inline void *bob_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr && size > 0) {
        fprintf(stderr, "Fatal error: Out of memory (requested %zu bytes)\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static inline void *bob_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr && count > 0 && size > 0) {
        fprintf(stderr, "Fatal error: Out of memory\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

static inline void *bob_realloc(void *ptr, size_t size) {
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr && size > 0) {
        fprintf(stderr, "Fatal error: Out of memory on realloc\n");
        exit(EXIT_FAILURE);
    }
    return new_ptr;
}

static inline char *bob_strdup(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = (char *)bob_malloc(len);
    memcpy(dup, s, len);
    return dup;
}

/* Dynamic string buffer */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} StringBuilder;

static inline StringBuilder *sb_create(void) {
    StringBuilder *sb = (StringBuilder *)bob_malloc(sizeof(StringBuilder));
    sb->capacity = 64;
    sb->length = 0;
    sb->data = (char *)bob_malloc(sb->capacity);
    sb->data[0] = '\0';
    return sb;
}

static inline void sb_append_len(StringBuilder *sb, const char *str, size_t len) {
    if (!str || len == 0) return;
    if (sb->length + len + 1 >= sb->capacity) {
        while (sb->length + len + 1 >= sb->capacity) {
            sb->capacity *= 2;
        }
        sb->data = (char *)bob_realloc(sb->data, sb->capacity);
    }
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

static inline void sb_append(StringBuilder *sb, const char *str) {
    if (str) {
        sb_append_len(sb, str, strlen(str));
    }
}

static inline void sb_append_char(StringBuilder *sb, char c) {
    char buf[2] = {c, '\0'};
    sb_append_len(sb, buf, 1);
}

static inline void sb_free(StringBuilder *sb) {
    if (sb) {
        free(sb->data);
        free(sb);
    }
}

#endif /* BOB_COMMON_H */
