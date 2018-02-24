#include "game.h"
#include "app.h"

#include "imgui.h"

struct Game {
   GameData data;
};

Game* gameCreate() {
   return new Game();
}

GameData* gameData(Game* game) { return &game->data; }

static Texture* textTexture = nullptr;
bool show_demo_window = true;
bool show_another_window = false;


static void _mainMenu(Game* game, Window* wnd) {
   if (ImGui::BeginMainMenuBar()) {
      if (ImGui::BeginMenu("Debug")) {

         if (ImGui::MenuItem("Dialog Stats")) {
            windowAddGUI(wnd, "DialogStats", [=](Window*wnd) {

               bool p_open = true;
               if (ImGui::Begin("Dialog Stats", &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
                  ImGui::Text("Active Dialogs: %d", DEBUG_windowGetDialogCount(wnd));

                  if (ImGui::Button("Open a test dialog")) {
                     static int testest = 0;
                     auto label = format("Dialog Test##%d", testest++);

                     windowAddGUI(wnd, label.c_str(), [=](Window*wnd) {
                        bool p_open = true;
                        if (ImGui::Begin(label.c_str(), &p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
                           ImGui::Text("Hi!");
                        }
                        ImGui::End();
                        return p_open;
                     });
                  }
               }
               ImGui::End();
               return p_open;
            });
         }


         ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();
   }
}

void gameUpdate(Game* game, Window* wnd) {
   _mainMenu(game, wnd);

   if (!textTexture) {
      textTexture = textureCreateFromPath("goku.png", { RepeatType_CLAMP, FilterType_LINEAR });
   }

   if (ImGui::Begin("Test Texture")) {
      auto sz = ImGui::GetWindowContentRegionMax();
      ImGui::Image((ImTextureID)textureGetHandle(textTexture), ImVec2(sz.x, sz.y));
   }
   ImGui::End();

   // 1. Show a simple window.
   // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
   {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
      ImGui::ColorEdit3("clear color", (float*)&game->data.imgui.bgClearColor); // Edit 3 floats representing a color

      ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
         counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
   }

   // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
   if (show_another_window)
   {
      ImGui::Begin("Another Window", &show_another_window);
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me"))
         show_another_window = false;
      ImGui::End();
   }

   // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
   if (show_demo_window)
   {
      ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
      ImGui::ShowDemoWindow(&show_demo_window);
   }
}

void gameDestroy(Game* game) {
   delete game;
}