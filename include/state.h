#ifndef STATE_H
#define STATE_H

#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include <arrays.h>

struct layer {
    struct point_array points;
};

struct state {
    int screenWidth;
    int screenHeight;
    int editorWidth;
    int editorHeight;
    Vector2 editorOffset;
    Color backgroundColor;
    Color gridColor;
    Color dragColor;
    Color pointColor;
    Color selectedColor;

    struct {
        struct size_t_array selectedPointIndices;
        struct layer_array layers;
        size_t selectedLayerID;

        Vector2 gridSize;
        Vector2 gridDimensions;

        bool mouseDragging;
        Vector2 mouseDragStart;
        Vector2 mouseDragEnd;

        bool canPlacePoint;

        float zoom;

        bool askForSaveFilePath;
        char* saveFilePath;
    } editor;
};

#endif