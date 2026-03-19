#ifndef ARRAYS_H
#define ARRAYS_H

#include <stdlib.h>
#include <string.h>

#include <raylib.h>

struct point_array {
    size_t size;
    size_t capacity;
    Vector2* data;
};

struct layer_array {
    size_t size;
    size_t capacity;
    struct layer* data;
};

struct size_t_array {
    size_t size;
    size_t capacity;
    size_t* data;
};

#define array_push(arr, ...) do { \
    if ((arr)->size >= (arr)->capacity) { \
        (arr)->capacity = (arr)->capacity > 0 ? (arr)->capacity * 2 : 4; \
        (arr)->data = realloc((arr)->data, (arr)->capacity * sizeof(*(arr)->data)); \
    } \
    (arr)->data[(arr)->size++] = (__VA_ARGS__); \
} while(0)

#define array_pop(arr) do { \
    if ((arr)->size > 0) { \
        (arr)->size--; \
    } \
} while(0)

#define array_free(arr) do { \
    free((arr)->data); \
    (arr)->data = NULL; \
    (arr)->size = 0; \
    (arr)->capacity = 0; \
} while(0)

#define array_remove(arr, index) do { \
    if ((index) < (arr)->size) { \
        memmove(&(arr)->data[(index)], &(arr)->data[(index) + 1], ((arr)->size - (index) - 1) * sizeof(*(arr)->data)); \
        (arr)->size--; \
    } \
} while(0)

#define array_get(arr, index) ((arr)->data[(index)])
#define array_size(arr) ((arr)->size)
#define array_new(type) ((struct { size_t size; size_t capacity; type* data; }){ 0, 0, NULL })

#endif