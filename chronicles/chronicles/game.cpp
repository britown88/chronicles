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

static void _doStatsWindow(Window* wnd) {

   ImGuiWindowFlags flags =
      ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoTitleBar;

   auto sz = windowSize(wnd);

   ImGui::SetNextWindowPos(ImVec2(sz.x, ImGui::GetFrameHeightWithSpacing()), ImGuiCond_Always, ImVec2(1, 0));
   if (ImGui::Begin("Stats", nullptr, flags)) {
      auto txt = format("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      //auto txtSize = ImGui::CalcTextSize(txt.c_str());

      //ImGui::SameLine(ImGui::GetWindowContentRegionWidth() - txtSize.x);
      ImGui::Text(txt.c_str());
   }
   ImGui::End();
}

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
         if (ImGui::MenuItem("Goku")) {

            Texture *gokuTex = nullptr;

            windowAddGUI(wnd, "goku", [=](Window*wnd) mutable {
               bool p_open = true;

               if (!gokuTex) {
                  gokuTex = textureCreateFromPath("goku.png", { RepeatType_CLAMP, FilterType_LINEAR });
               }

               if (ImGui::Begin("goku", &p_open)) {
                  auto sz = ImGui::GetWindowContentRegionMax();
                  ImGui::Image((ImTextureID)textureGetHandle(gokuTex), ImVec2(sz.x, sz.y));
               }

               if (!p_open) {
                  textureDestroy(gokuTex);
               }

               ImGui::End();
               return p_open;
            });
         }
         if (ImGui::MenuItem("ImGui Demo")) {
            windowAddGUI(wnd, "imguidemo", [=](Window*wnd) mutable {
               bool p_open = true;
               ImGui::ShowDemoWindow(&p_open);               
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

   _doStatsWindow(wnd);

}

void gameDestroy(Game* game) {
   delete game;
}