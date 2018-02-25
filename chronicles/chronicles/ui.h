#pragma once

#include "ega.h"

typedef struct Window Window;
typedef struct GameData GameData;
void uiToolStartEGAEncoder(Window* wnd);

enum PaletteEditorFlags_ {
   PaletteEditorFlags_ENCODE = (1 << 0), // adds encode sentinel values to color picker
   PaletteEditorFlags_POPPED_OUT = (1 << 1)// Dont use, set by popout
};
typedef byte PaletteEditorFlags;

void uiPaletteEditor(Window* wnd, EGAPalette* pal, char* palName = nullptr, u32 palNameSize = 0, PaletteEditorFlags flags = 0);
float uiPaletteEditorWidth();
