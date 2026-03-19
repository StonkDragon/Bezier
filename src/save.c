#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

#include <save.h>
#include <state.h>
#include <arrays.h>

const int saveVersion = 3;

void saveState(struct state* state, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (file) {
        #define write_field(field) fwrite(&field, sizeof(field), 1, file)
        write_field(saveVersion);
        write_field(state->editor.gridDimensions);
        write_field(state->editor.gridSize);
        write_field(state->editor.layers.size);
        for (size_t i = 0; i < array_size(&state->editor.layers); i++) {
            struct layer s = array_get(&state->editor.layers, i);
            write_field(s.points.size);
            fwrite(s.points.data, sizeof(Vector2), array_size(&s.points), file);
        }
        #undef write_field

        fclose(file);
    }
}

void saveSvg(struct state* state) {
    FILE* file = fopen(state->editor.saveFilePath, "w");
    if (file) {
        float gridStartX = (state->editorWidth / 2) - (state->editor.gridDimensions.x / 2);
        float gridStartY = (state->editorHeight / 2) - (state->editor.gridDimensions.y / 2);

        fprintf(file, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\">\n", (int) state->editor.gridDimensions.x, (int) state->editor.gridDimensions.y);
        for (size_t i = 0; i < array_size(&state->editor.layers); i++) {
            struct layer s = array_get(&state->editor.layers, i);
            for (size_t j = 0; j + 2 < array_size(&s.points); j += 2) {
                Vector2 start = array_get(&s.points, j);
                Vector2 control = array_get(&s.points, j + 1);
                Vector2 end = array_get(&s.points, j + 2);
                fprintf(file, "  <path d=\"M %.2f %.2f Q %.2f %.2f %.2f %.2f\" stroke=\"white\" fill=\"none\" />\n",
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

#define read_field(field) fread(&field, sizeof(field), 1, file)

static void loadSaveStateVersion1(struct state* state, FILE* file) {
    size_t layerCount;
    read_field(layerCount);
    for (size_t i = 0; i < layerCount; i++) {
        struct layer newLayer = { 0, 0, NULL };
        array_push(&state->editor.layers, newLayer);
        struct layer* s = &array_get(&state->editor.layers, array_size(&state->editor.layers) - 1);

        size_t pointCount = 0;
        read_field(pointCount);
        for (size_t j = 0; j < pointCount; j++) {
            Vector2 point;
            fread(&point, sizeof(Vector2), 1, file);
            array_push(&s->points, point);
        }
    }
}

static void loadSaveStateVersion2(struct state* state, FILE* file) {
    read_field(state->editor.gridDimensions);
    loadSaveStateVersion1(state, file);
}

static void loadSaveStateVersion3(struct state* state, FILE* file) {
    read_field(state->editor.gridDimensions);
    read_field(state->editor.gridSize);
    loadSaveStateVersion1(state, file);
}

void loadState(struct state* state, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file) {
        int loadedSaveVersion = 0;
        read_field(loadedSaveVersion);
        printf("Loaded save version: %d\n", loadedSaveVersion);
        switch (loadedSaveVersion) {
            case 1:
                loadSaveStateVersion1(state, file);
                break;
            case 2:
                loadSaveStateVersion2(state, file);
                break;
            case 3:
                loadSaveStateVersion3(state, file);
                break;
            default:
                fprintf(stderr, "Unknown save version: %d\n", loadedSaveVersion);
                break;
        }
        
        fclose(file);
    }
}

#undef read_field

void quickSave(struct state* state) {
    struct timespec ts = { 0, 0 };
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm tm = { 0 };
    localtime_r(&ts.tv_sec, &tm);

    saveState(state, TextFormat("quicksave-%d.%02d.%02d-%02d:%02d:%02d.dat", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec));
}

void loadLastQuickSave(struct state* state) {
    // filter files in current directory for ones that start with "quicksave-" and end with ".dat", then find the one with the last modified time and load it
    const char* savePrefix = "quicksave-";
    const char* saveSuffix = ".dat";
    DIR* dir = opendir(".");
    if (dir) {
        struct dirent* entry = NULL;
        char latestSaveFile[256];
        time_t latestSaveTime = 0;
        while ((entry = readdir(dir)) != NULL) {
            printf("Found file: %s\n", entry->d_name);
            if (strncmp(entry->d_name, savePrefix, strlen(savePrefix)) == 0 && strcmp(entry->d_name + strlen(entry->d_name) - strlen(saveSuffix), saveSuffix) == 0) {
                struct stat st = { 0 };
                if (stat(entry->d_name, &st) == 0) {
                    if (st.st_mtime > latestSaveTime) {
                        latestSaveTime = st.st_mtime;
                        strncpy(latestSaveFile, entry->d_name, sizeof(latestSaveFile));
                        latestSaveFile[sizeof(latestSaveFile) - 1] = '\0';
                    }
                }
            }
        }
        closedir(dir);
        if (latestSaveTime > 0) {
            loadState(state, latestSaveFile);
            fprintf(stderr, "Loaded quicksave: %s\n", latestSaveFile);
            return;
        }
    }
    fprintf(stderr, "No quicksave found, starting with empty state.\n");
}
