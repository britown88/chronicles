#pragma once

#include "defs.h"
#include "math.h"

#include <functional>

//fuck it we'll put all our app stuff here!

// APP
typedef struct App App;
App* appCreate();

typedef struct {
   u32 w, h;
   StringView title;
}WindowConfig;

void appCreateWindow(App* app, WindowConfig const& info);

bool appRunning(App* app);
//void appPollEvents(App* app);
void appStep(App* app);

void appDestroy(App* app);

// Window
typedef struct Window Window;
Int2 windowSize(Window* wnd);
Int2 windowClientArea(Window* wnd); // windowSize * scaleFactor
float windowScale(Window* wnd); //set by dpi

// gui(wnd) is called every frame during the imgui update until it returns false
// do your one-off imgui::begin()s here, use lambda capture for state
// label must be unique or this call is ignored
void windowAddGUI(Window* wnd, StringView label, std::function<bool(Window*)> const && gui);


// Rendering
enum {
   RepeatType_REPEAT,
   RepeatType_CLAMP
};
typedef byte RepeatType;

enum {
   FilterType_LINEAR,
   FilterType_NEAREST
};
typedef byte FilterType;

typedef struct {
   RepeatType repeatType = RepeatType_CLAMP;
   FilterType filterType = FilterType_NEAREST;
} TextureConfig;

typedef struct Texture Texture;

Texture *textureCreateFromPath(StringView path, TextureConfig const& config);
Texture *textureCreateFromBuffer(byte* buffer, u64 size, TextureConfig const& config);
Texture *textureCreateCustom(u32 width, u32 height, TextureConfig const& config);
void textureDestroy(Texture *self);

void textureSetPixels(Texture *self, byte *data);
Int2 textureGetSize(Texture *t);

//because why not
u32 textureGetHandle(Texture *self);

const ColorRGBA *textureGetPixels(Texture *self);

