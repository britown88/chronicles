#include "app.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <GL/glew.h> 
#include <imgui.h>
#include <stb/stb_image.h>

#include "imgui_impl_sdl_gl3.h"
#include "game.h"

#include "math.h"

struct Window {
   SDL_Window* sdlWnd = nullptr;
   SDL_GLContext sdlCtx = nullptr;
   ImGuiContext* imguiContext = nullptr;

   Int2 size = { 0 }, clientSize = { 0 };
   float scale = 1.0f;
};

struct App {
   bool running = false;
   Window* wnd = nullptr;

   Game* game;   
};

App* appCreate() {
   auto out = new App();
   out->game = gameCreate();
   return out;
}

void appDestroy(App* app) {
   gameDestroy(app->game);

   ImGui_ImplSdlGL3_Shutdown();
   ImGui::DestroyContext();

   SDL_GL_DeleteContext(app->wnd->sdlCtx);
   SDL_DestroyWindow(app->wnd->sdlWnd);
   SDL_Quit();

   delete app->wnd;
   delete app;
}

static Window* _windowCreate(WindowConfig const& info) {
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0)
   {
      printf("Error: %s\n", SDL_GetError());
      return nullptr;
   }

   // Setup window
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);

   SDL_DisplayMode current;
   SDL_GetCurrentDisplayMode(0, &current);

   auto winFlags = 
      SDL_WINDOW_OPENGL | 
      SDL_WINDOW_RESIZABLE |
      SDL_WINDOW_MAXIMIZED;

   SDL_Window *window = SDL_CreateWindow(info.title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, info.w, info.h, winFlags);
   SDL_GLContext glcontext = SDL_GL_CreateContext(window);
   SDL_GL_SetSwapInterval(1); // Enable vsync
   glewInit();

   

   // Setup ImGui binding
   auto imCtx = ImGui::CreateContext();
   ImGuiIO& io = ImGui::GetIO(); (void)io;
   ImGui_ImplSdlGL3_Init(window);

   ImGui::StyleColorsDark();

   Window* out = new Window();
   out->sdlCtx = glcontext;
   out->sdlWnd = window;
   out->imguiContext = imCtx;
   windowRefreshSize(out);

   return out;
}

void appCreateWindow(App* app, WindowConfig const& info) {
   app->wnd = _windowCreate(info);
   app->running = true;
}

bool appRunning(App* app) { return app->running; }
void appPollEvents(App* app) {
   SDL_Event event;
   while (SDL_PollEvent(&event))
   {
      ImGui_ImplSdlGL3_ProcessEvent(app->wnd, &event);

      switch (event.type)
      {
      case SDL_QUIT: {
         app->running = false;
         break; }
      case SDL_MOUSEBUTTONDOWN: {

         break; }
      case SDL_TEXTINPUT: {

         break; }
      case SDL_KEYDOWN:
      case SDL_KEYUP: {
         switch (event.key.keysym.scancode) {
         case SDL_SCANCODE_ESCAPE:
            app->running = false;
            break;
         }
         break;  }
      case SDL_WINDOWEVENT:
         switch (event.window.event) {
         case SDL_WINDOWEVENT_EXPOSED:
         case SDL_WINDOWEVENT_RESIZED:
            windowRefreshSize(app->wnd);
            break;
         }
         break;
      }
   }
}



static void _pollEvents(App* app) {
   appPollEvents(app);
}

static void _beginFrame(App* app) {
   ImGui_ImplSdlGL3_NewFrame(app->wnd->sdlWnd);
}

static void _updateGame(App* app) {
   gameUpdate(app->game, app->wnd);
}

static void _renderFrame(App* app) {
   auto data = gameData(app->game);
   auto ccolor = data->imgui.bgClearColor;

   glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
   glClearColor(ccolor.r, ccolor.g, ccolor.b, 1.0f);
   glClear(GL_COLOR_BUFFER_BIT);
   ImGui::Render();
   ImGui_ImplSdlGL3_RenderDrawData(ImGui::GetDrawData());
   SDL_GL_SwapWindow(app->wnd->sdlWnd);
}

void appStep(App* app) {   
   _pollEvents(app);
   _beginFrame(app);
   _updateGame(app);
   _renderFrame(app);
}


// Window
Int2 windowSize(Window* wnd) { return wnd->size; }
Int2 windowClientArea(Window* wnd) { return wnd->clientSize; }
float windowScale(Window* wnd) { return wnd->scale; }

void windowRefreshSize(Window* wnd) {
   SDL_GetWindowSize(wnd->sdlWnd, &wnd->size.x, &wnd->size.y);
   SDL_GL_GetDrawableSize(wnd->sdlWnd, &wnd->clientSize.x, &wnd->clientSize.y);
   wnd->scale = wnd->clientSize.x / (float)wnd->size.x;
}

void windowAddGUI(Window* wnd, StringView label, std::function<bool(Window*)> const && gui) {

}


struct Texture {
   enum {
      SourceType_PATH,
      SourceType_BUFFER,
      SourceType_CUSTOM
   };
   typedef byte SourceType;

   SourceType srcType = SourceType_PATH;
   std::string path;
   byte* buffer = nullptr;
   u64 bufferSize = 0;
   TextureFromBufferFlag buffFlag = 0;

   TextureConfig config = { 0 };
   bool isLoaded = false;

   GLuint glHandle = 0;
   ColorRGBA *pixels = nullptr;
   Int2 size = { 0 };

   bool dirty = true;
};

static void _textureRelease(Texture *self) {
   if (self->isLoaded) {
      glDeleteTextures(1, &self->glHandle);
   }

   delete[] self->pixels;

   self->glHandle = -1;
   self->isLoaded = false;
}
static void _textureAcquire(Texture *self) {

   byte* data = nullptr;

   switch (self->srcType) {
   case Texture::SourceType_PATH: {
      int comps = 0;
      data = stbi_load(self->path.c_str(), &self->size.x, &self->size.y, &comps, 4);      
      break; }
   case Texture::SourceType_BUFFER: {
      int comps = 0;
      data = stbi_load_from_memory(self->buffer, (int32_t)self->bufferSize, &self->size.x, &self->size.y, &comps, 4);
      break; }
   }

   if (data) {
      int pixelCount = self->size.x * self->size.y;
      self->pixels = new ColorRGBA[pixelCount];
      memcpy(self->pixels, data, pixelCount * sizeof(ColorRGBA));

      stbi_image_free(data);
   }
   
   if (!self->pixels) {
      return;
   }

   glEnable(GL_TEXTURE_2D);
   glGenTextures(1, &self->glHandle);
   glBindTexture(GL_TEXTURE_2D, self->glHandle);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   switch (self->config.filterType) {
   case FilterType_LINEAR:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      break;
   case FilterType_NEAREST:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      break;
   };

   switch (self->config.repeatType) {
   case RepeatType_REPEAT:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      break;
   case RepeatType_CLAMP:
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      break;
   };

   //glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, self->size.x, self->size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, self->pixels);
   glBindTexture(GL_TEXTURE_2D, 0);

   self->isLoaded = true;
}


Texture *textureCreateFromPath(StringView path, TextureConfig const& config) {
   int x = 0, y = 0, comp = 0;

   stbi_info(path, &x, &y, &comp);

   if (x == 0 && y == 0) {
      return NULL;
   }

   Texture *out = new Texture();
   out->config = config;
   out->srcType = Texture::SourceType_PATH;
   out->path = path;

   out->size.x = x;
   out->size.y = y;
   out->glHandle = -1;

   return out;
}
Texture *textureCreateFromBuffer(byte* buffer, u64 size, TextureConfig const& config, TextureFromBufferFlag flag) {
   int x = 0, y = 0, comp = 0;

   stbi_info_from_memory(buffer, (int32_t)size, &x, &y, &comp);
   if (x == 0 && y == 0) {
      return NULL;
   }

   Texture *out = new Texture();
   out->config = config;
   out->srcType = Texture::SourceType_BUFFER;
   
   out->bufferSize = size;
   out->buffFlag = flag;

   if (flag == TextureFromBufferFlag_COPY) {
      out->buffer = new byte[size];
      memcpy(out->buffer, buffer, size);
   }
   else {
      out->buffer = buffer;
   }

   out->size.x = x;
   out->size.y = y;
   out->glHandle = -1;

   return out;
}
Texture *textureCreateCustom(u32 width, u32 height, TextureConfig const& config) {
   Texture* out = new Texture();

   out->config = config;
   out->srcType = Texture::SourceType_CUSTOM;

   out->size.x = width;
   out->size.y = height;

   out->pixels = new ColorRGBA[width*height];
   return out;
}
void textureDestroy(Texture *self) {
   if (self->pixels) {
      _textureRelease(self);
   }

   if (self->srcType == Texture::SourceType_BUFFER &&
         (self->buffFlag == TextureFromBufferFlag_TAKE_OWNERHSIP ||
          self->buffFlag == TextureFromBufferFlag_COPY)) {

      delete[] self->buffer;
   }

   delete self;
}

void textureSetPixels(Texture *self, byte *data) {
   memcpy(self->pixels, data, self->size.x * self->size.y * sizeof(ColorRGBA));
   self->dirty = true;
}
Int2 textureGetSize(Texture *t) {
   return t->size;
}

//because why not
u32 textureGetHandle(Texture *self) {
   if (!self->isLoaded) {
      _textureAcquire(self);
   }

   if (self->dirty) {
      glBindTexture(GL_TEXTURE_2D, self->glHandle);
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, self->size.x, self->size.y, GL_RGBA, GL_UNSIGNED_BYTE, self->pixels);
      glBindTexture(GL_TEXTURE_2D, 0);
      self->dirty = false;
   }

   return self->glHandle;
}

const ColorRGBA *textureGetPixels(Texture *self) {
   if (!self->isLoaded) {
      _textureAcquire(self);
   }
   return self->pixels;
}