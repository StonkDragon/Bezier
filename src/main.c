#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <raylib.h>
#include <raymath.h>

struct point_array {
    size_t size;
    size_t capacity;
    Vector2* data;
};

struct layer {
    struct point_array points;
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
    } editor;
};

void DrawBezierQuad(Vector2 start, Vector2 control, Vector2 end, int segments, Color color) {
    for (int i = 0; i < segments; i++) {
        float t1 = (float)i / segments;
        float t2 = (float)(i + 1) / segments;

        Vector2 p1 = Vector2Lerp(
            Vector2Lerp(start, control, t1),
            Vector2Lerp(control, end, t1),
            t1
        );

        Vector2 p2 = Vector2Lerp(
            Vector2Lerp(start, control, t2),
            Vector2Lerp(control, end, t2),
            t2
        );

        DrawLineV(p1, p2, color);
    }
}

Vector2 editorPosToScreenPos(struct state* state, Vector2 editorPos) {
    float startX = (state->screenWidth - state->editorWidth * state->editor.zoom) / 2.0f + state->editorOffset.x;
    float startY = (state->screenHeight - state->editorHeight * state->editor.zoom) / 2.0f + state->editorOffset.y;

    return (Vector2){
        .x = startX + editorPos.x * state->editor.zoom,
        .y = startY + editorPos.y * state->editor.zoom,
    };
}

int main() {
    struct state state = {
        .screenWidth = 1600,
        .screenHeight = 900,
        .editorWidth = 1600,
        .editorHeight = 900,

        .editorOffset = { 0, 0 },

        .backgroundColor = (Color){ 0x10, 0x10, 0x10, 0xFF },
        .gridColor = (Color){ 0x40, 0x40, 0x40, 0xFF },
        .dragColor = (Color){ 0x20, 0x20, 0x20, 0xFF },
        .pointColor = (Color){ 0xFF, 0xFF, 0xFF, 0xFF },
        .selectedColor = (Color){ 0x00, 0xFF, 0x00, 0xFF },
        .editor = {
            .selectedPointIndices = { 0, 0, NULL },

            .layers = { 0, 0, NULL },

            .gridSize = { 10, 10 },
            .gridDimensions = { 500, 500 },

            .mouseDragging = false,
            .mouseDragStart = { 0, 0 },
            .mouseDragEnd = { 0, 0 },

            .canPlacePoint = true,

            .zoom = 1.0f,
        },
    };

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    
    InitWindow(state.screenWidth, state.screenHeight, "Bezier Curve Editor");

    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));

    SetExitKey(KEY_NULL);

    Font defaultFont = GetFontDefault();

    while (!WindowShouldClose()) {
        BeginDrawing();

        if (IsWindowResized()) {
            state.screenWidth = GetScreenWidth();
            state.screenHeight = GetScreenHeight();
        }

        float gridStartX = (state.editorWidth / 2) - (state.editor.gridDimensions.x / 2);
        float gridStartY = (state.editorHeight / 2) - (state.editor.gridDimensions.y / 2);
        float gridEndX = gridStartX + state.editor.gridDimensions.x;
        float gridEndY = gridStartY + state.editor.gridDimensions.y;

        bool leftMouseButtonDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        Vector2 position = { -1, -1 };
        if (leftMouseButtonDown) {
            position = GetMousePosition();
            position.x = (position.x - (state.screenWidth - state.editorWidth * state.editor.zoom) / 2.0f) / state.editor.zoom - state.editorOffset.x / state.editor.zoom;
            position.y = (position.y - (state.screenHeight - state.editorHeight * state.editor.zoom) / 2.0f) / state.editor.zoom - state.editorOffset.y / state.editor.zoom;
        }
        
        if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            if (position.x < gridStartX) position.x = gridStartX;
            if (position.x > gridEndX) position.x = gridEndX;
            if (position.y < gridStartY) position.y = gridStartY;
            if (position.y > gridEndY) position.y = gridEndY;

            if (fmodf(position.x - gridStartX, state.editor.gridSize.x) < state.editor.gridSize.x / 2) {
                position.x = gridStartX + floorf((position.x - gridStartX) / state.editor.gridSize.x) * state.editor.gridSize.x;
            } else {
                position.x = gridStartX + ceilf((position.x - gridStartX) / state.editor.gridSize.x) * state.editor.gridSize.x;
            }
            if (fmodf(position.y - gridStartY, state.editor.gridSize.y) < state.editor.gridSize.y / 2) {
                position.y = gridStartY + floorf((position.y - gridStartY) / state.editor.gridSize.y) * state.editor.gridSize.y;
            } else {
                position.y = gridStartY + ceilf((position.y - gridStartY) / state.editor.gridSize.y) * state.editor.gridSize.y;
            }
        }

        if (GetMouseWheelMove() < 0) {
            state.editor.zoom -= 0.1f;
            if (state.editor.zoom < 0.1f) state.editor.zoom = 0.1f;
        } else if (GetMouseWheelMove() > 0) {
            state.editor.zoom += 0.1f;
            if (state.editor.zoom > 10.0f) state.editor.zoom = 10.0f;
        }

        struct layer* selectedLayer = NULL;
        if (state.editor.selectedLayerID >= 0 && state.editor.selectedLayerID < array_size(&state.editor.layers)) {
            selectedLayer = &array_get(&state.editor.layers, state.editor.selectedLayerID);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            state.editor.mouseDragging = false;
            if (selectedLayer != NULL && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
                state.editor.mouseDragging = true;
                state.editor.mouseDragEnd = position;
                if (state.editor.mouseDragStart.x == -1 && state.editor.mouseDragStart.y == -1) {
                    state.editor.mouseDragStart = position;
                }

                Rectangle selectionRect = {
                    .x = fmin(state.editor.mouseDragStart.x, position.x),
                    .y = fmin(state.editor.mouseDragStart.y, position.y),
                    .width = fabs(position.x - state.editor.mouseDragStart.x),
                    .height = fabs(position.y - state.editor.mouseDragStart.y),
                };
                state.editor.selectedPointIndices.size = 0; // Clear previous selection
                for (size_t i = 0; i < array_size(&selectedLayer->points); i++) {
                    Vector2 p = array_get(&selectedLayer->points, i);
                    if (CheckCollisionPointRec(p, selectionRect)) {
                        array_push(&state.editor.selectedPointIndices, i);
                    }
                }

            } else if (IsKeyDown(KEY_SPACE)) {
                state.editorOffset = Vector2Add(state.editorOffset, GetMouseDelta());
            } else if (selectedLayer != NULL && state.editor.canPlacePoint) {
                if (
                    position.x >= gridStartX && position.x <= gridEndX &&
                    position.y >= gridStartY && position.y <= gridEndY
                ) {
                    state.editor.canPlacePoint = false;
                    array_push(&selectedLayer->points, position);
                }
            }
        } else if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            state.editor.mouseDragging = false;
            state.editorOffset = Vector2Add(state.editorOffset, GetMouseDelta());
        } else {
            state.editor.mouseDragging = false;
            state.editor.canPlacePoint = true;
        }

        if (!state.editor.mouseDragging) {
            state.editor.mouseDragStart = (Vector2){ -1, -1 };
        }

        if (IsKeyPressed(KEY_DELETE)) {
            if (selectedLayer != NULL) {
                if (array_size(&state.editor.selectedPointIndices) == 0) {
                    // If no points are selected, delete the last point of the layer. If that was the last point, delete the layer.
                    if (array_size(&selectedLayer->points) > 0) {
                        array_pop(&selectedLayer->points);
                    } else {
                        array_remove(&state.editor.layers, state.editor.selectedLayerID);
                        if (state.editor.selectedLayerID >= array_size(&state.editor.layers)) {
                            state.editor.selectedLayerID = array_size(&state.editor.layers) - 1;
                        }
                    }
                } else {
                    size_t removedCount = 0;
                    for (size_t i = 0; i < array_size(&state.editor.selectedPointIndices); i++) {
                        size_t index = array_get(&state.editor.selectedPointIndices, i) - removedCount; // adjust index for already removed points
                        if (index >= 0 && index < array_size(&selectedLayer->points)) {
                            array_remove(&selectedLayer->points, index);
                            removedCount++;
                        }
                    }
                    array_free(&state.editor.selectedPointIndices);
                }
            }
        } else if (IsKeyPressed(KEY_DOWN)) {
            if (state.editor.selectedLayerID < array_size(&state.editor.layers) - 1) {
                state.editor.selectedLayerID++;
                state.editor.selectedPointIndices.size = 0; // Clear selected points when changing layer
            }
        } else if (IsKeyPressed(KEY_UP)) {
            if (state.editor.selectedLayerID > 0) {
                state.editor.selectedLayerID--;
                state.editor.selectedPointIndices.size = 0; // Clear selected points when changing layer
            }
        } else if (IsKeyPressed(KEY_N)) {
            struct layer newLayer = { 0, 0, NULL };
            array_push(&state.editor.layers, newLayer);
            state.editor.selectedLayerID = array_size(&state.editor.layers) - 1;
        } else if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            if (selectedLayer != NULL && IsKeyPressed(KEY_A)) {
                for (size_t i = 0; i < array_size(&selectedLayer->points); i++) {
                    array_push(&state.editor.selectedPointIndices, i);
                }
            }
        }

        ClearBackground(state.backgroundColor);

        for (int x = gridStartX; x < gridEndX; x += state.editor.gridSize.x) {
            Vector2 start = editorPosToScreenPos(&state, (Vector2){ x, gridStartY });
            Vector2 end = editorPosToScreenPos(&state, (Vector2){ x, gridEndY });
            DrawLineV(start, end, state.gridColor);
        }
        for (int y = gridStartY; y < gridEndY; y += state.editor.gridSize.y) {
            Vector2 start = editorPosToScreenPos(&state, (Vector2){ gridStartX, y });
            Vector2 end = editorPosToScreenPos(&state, (Vector2){ gridEndX, y });
            DrawLineV(start, end, state.gridColor);
        }

        if (state.editor.mouseDragging) {
            Vector2 dragStart = editorPosToScreenPos(&state, state.editor.mouseDragStart);
            Vector2 dragEnd = editorPosToScreenPos(&state, state.editor.mouseDragEnd);

            Rectangle selectionRect = {
                .x = fmin(dragStart.x, dragEnd.x),
                .y = fmin(dragStart.y, dragEnd.y),
                .width = fabs(dragEnd.x - dragStart.x),
                .height = fabs(dragEnd.y - dragStart.y),
            };

            DrawRectangleLinesEx(selectionRect, 2, state.dragColor);
        }

        if (selectedLayer != NULL) {
            for (size_t i = 0; i < array_size(&selectedLayer->points); i++) {
                Vector2 point = editorPosToScreenPos(&state, array_get(&selectedLayer->points, i));
                bool isSelected = false;

                for (size_t j = 0; j < array_size(&state.editor.selectedPointIndices); j++) {
                    size_t selectedIndex = array_get(&state.editor.selectedPointIndices, j);
                    if (selectedIndex == i) {
                        isSelected = true;
                        break;
                    }
                }

                if (isSelected) {
                    DrawCircleV(point, 5, state.selectedColor);
                } else {
                    DrawCircleV(point, 5, state.pointColor);
                }
            }

            if (array_size(&selectedLayer->points) > 2) {
                for (size_t i = 0; i + 2 < array_size(&selectedLayer->points); i += 2) {
                    Vector2 start = array_get(&selectedLayer->points, i);
                    Vector2 control = array_get(&selectedLayer->points, i + 1);
                    Vector2 end = array_get(&selectedLayer->points, i + 2);
                    start = editorPosToScreenPos(&state, start);
                    control = editorPosToScreenPos(&state, control);
                    end = editorPosToScreenPos(&state, end);
                    DrawBezierQuad(start, control, end, 20, state.pointColor);
                }
            }
        }

        for (size_t i = 0; i < array_size(&state.editor.layers); i++) {
            if (selectedLayer != NULL && i == state.editor.selectedLayerID) continue; // Skip selected layer since it's drawn on top
            struct layer s = array_get(&state.editor.layers, i);
            for (size_t j = 0; j + 2 < array_size(&s.points); j += 2) {
                Vector2 start = array_get(&s.points, j);
                Vector2 control = array_get(&s.points, j + 1);
                Vector2 end = array_get(&s.points, j + 2);
                start = editorPosToScreenPos(&state, start);
                control = editorPosToScreenPos(&state, control);
                end = editorPosToScreenPos(&state, end);
                DrawBezierQuad(start, control, end, 20, state.pointColor);
            }
        }

        DrawText(TextFormat("Zoom: %.2f", state.editor.zoom), 10, 10, 20, state.pointColor);
        
        for (size_t i = 0; i < array_size(&state.editor.layers); i++) {
            struct layer s = array_get(&state.editor.layers, i);
            Color textColor = (selectedLayer != NULL && i == state.editor.selectedLayerID) ? state.selectedColor : state.pointColor;
            
            const char* layerLabel = TextFormat("Layer %zu: %zu points", i + 1, array_size(&s.points));
            Vector2 textBounds = MeasureTextEx(defaultFont, layerLabel, 20, 1);
            Vector2 textPosition = { 10, 40 + 30 * i };
            Rectangle textBackground = { textPosition.x - 5, textPosition.y - 5, textBounds.x + 10, textBounds.y + 10 };
            if (CheckCollisionPointRec(GetMousePosition(), textBackground)) {
                textColor = state.selectedColor;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    state.editor.selectedLayerID = i;
                    state.editor.selectedPointIndices.size = 0; // Clear selected points when changing layer
                }
            }

            DrawText(layerLabel, textPosition.x, textPosition.y, 20, textColor);
        }

        if (IsKeyPressed(KEY_S)) {
            const char* filename = TextFormat("bezier_editor_save_%lld.svg", (long long) time(NULL));
            FILE* file = fopen(filename, "w");
            if (file) {
                fprintf(file, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\">\n", (int) state.editor.gridDimensions.x, (int) state.editor.gridDimensions.y);
                for (size_t i = 0; i < array_size(&state.editor.layers); i++) {
                    struct layer s = array_get(&state.editor.layers, i);
                    for (size_t j = 0; j + 2 < array_size(&s.points); j += 2) {
                        Vector2 start = array_get(&s.points, j);
                        Vector2 control = array_get(&s.points, j + 1);
                        Vector2 end = array_get(&s.points, j + 2);
                        fprintf(file, "  <path d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\" stroke=\"white\" fill=\"transparent\" />\n",
                            start.x - gridStartX, start.y - gridStartY,
                            control.x - gridStartX, control.y - gridStartY,
                            end.x - gridStartX, end.y - gridStartY
                        );
                    }
                }
                fprintf(file, "</svg>\n");
                fclose(file);
            }
        }

        DrawFPS(10, 10 + 30 * array_size(&state.editor.layers) + 30);

        EndDrawing();
    }

    return 0;
}
