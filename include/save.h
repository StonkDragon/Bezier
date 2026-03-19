#ifndef SAVE_H
#define SAVE_H

#include <state.h>

void saveState(struct state* state, const char* filename);
void saveSvg(struct state* state);
void loadState(struct state* state, const char* filename);
void quickSave(struct state* state);
void loadLastQuickSave(struct state* state);

#endif