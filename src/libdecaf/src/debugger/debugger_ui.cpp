#include "debugger_ui.h"
#include <imgui.h>

using namespace decaf::input;

namespace debugger
{

namespace ui
{

static ClipboardTextGetCallback
sGetClipboardText = nullptr;

static ClipboardTextSetCallback
sSetClipboardText = nullptr;

static bool
sMousePressed[3] = { false, false, false };

static bool
sMouseClicked[3] = { false, false, false };

static float
sMouseWheel = 0.0f;

static float
sMousePosX = 0.0f, sMousePosY = 0.0f;

void
initialise()
{
   auto &io = ImGui::GetIO();

   io.GetClipboardTextFn = []() {
      return sGetClipboardText ? sGetClipboardText() : "";
   };

   io.SetClipboardTextFn = [](const char *text) {
      if (sSetClipboardText) {
         sSetClipboardText(text);
      }
   };

   io.KeyMap[ImGuiKey_Tab] = static_cast<int>(KeyboardKey::Tab);
   io.KeyMap[ImGuiKey_LeftArrow] = static_cast<int>(KeyboardKey::LeftArrow);
   io.KeyMap[ImGuiKey_RightArrow] = static_cast<int>(KeyboardKey::RightArrow);
   io.KeyMap[ImGuiKey_UpArrow] = static_cast<int>(KeyboardKey::UpArrow);
   io.KeyMap[ImGuiKey_DownArrow] = static_cast<int>(KeyboardKey::DownArrow);
   io.KeyMap[ImGuiKey_PageUp] = static_cast<int>(KeyboardKey::PageUp);
   io.KeyMap[ImGuiKey_PageDown] = static_cast<int>(KeyboardKey::PageDown);
   io.KeyMap[ImGuiKey_Home] = static_cast<int>(KeyboardKey::Home);
   io.KeyMap[ImGuiKey_End] = static_cast<int>(KeyboardKey::End);
   io.KeyMap[ImGuiKey_Delete] = static_cast<int>(KeyboardKey::Delete);
   io.KeyMap[ImGuiKey_Backspace] = static_cast<int>(KeyboardKey::Backspace);
   io.KeyMap[ImGuiKey_Enter] = static_cast<int>(KeyboardKey::Enter);
   io.KeyMap[ImGuiKey_Escape] = static_cast<int>(KeyboardKey::Escape);
   io.KeyMap[ImGuiKey_A] = static_cast<int>(KeyboardKey::A);
   io.KeyMap[ImGuiKey_C] = static_cast<int>(KeyboardKey::C);
   io.KeyMap[ImGuiKey_V] = static_cast<int>(KeyboardKey::V);
   io.KeyMap[ImGuiKey_X] = static_cast<int>(KeyboardKey::X);
   io.KeyMap[ImGuiKey_Y] = static_cast<int>(KeyboardKey::Y);
   io.KeyMap[ImGuiKey_Z] = static_cast<int>(KeyboardKey::Z);

   io.Fonts->AddFontDefault();

   static const ImWchar icons_ranges[] = { 0x2500, 0x25ff, 0 };
   ImFontConfig config;
   config.MergeMode = true;
   config.MergeGlyphCenterV = true;
   io.Fonts->AddFontFromFileTTF("resources/fonts/DejaVuSansMono.ttf", 13.0f, &config, icons_ranges);

   auto &style = ImGui::GetStyle();
   style.Colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.95f);
}

void
injectMouseButtonInput(MouseButton button, MouseAction action)
{
   if (button == MouseButton::Unknown) {
      return;
   }

   auto id = static_cast<int>(button);

   if (action == MouseAction::Press) {
      sMousePressed[id] = true;
      sMouseClicked[id] = true;
   } else if (action == MouseAction::Release) {
      sMousePressed[id] = false;
   }
}

void
injectMousePos(float x, float y)
{
   sMousePosX = x;
   sMousePosY = y;
}

void
injectScrollInput(float xoffset, float yoffset)
{
   sMouseWheel += yoffset;
}

void
injectKeyInput(KeyboardKey key, KeyboardAction action)
{
   auto &io = ImGui::GetIO();
   auto idx = static_cast<int>(key);

   if (action == KeyboardAction::Press) {
      io.KeysDown[idx] = true;
   }

   if (action == KeyboardAction::Release) {
      io.KeysDown[idx] = false;
   }

   io.KeyCtrl =
      io.KeysDown[static_cast<int>(KeyboardKey::LeftControl)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightControl)];

   io.KeyShift =
      io.KeysDown[static_cast<int>(KeyboardKey::LeftShift)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightShift)];

   io.KeyAlt =
      io.KeysDown[static_cast<int>(KeyboardKey::LeftAlt)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightAlt)];

   io.KeySuper =
      io.KeysDown[static_cast<int>(KeyboardKey::LeftSuper)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightSuper)];
}

void
injectTextInput(const char *text)
{
   auto &io = ImGui::GetIO();
   io.AddInputCharactersUTF8(text);
}

void
setClipboardTextCallbacks(ClipboardTextGetCallback getter,
                          ClipboardTextSetCallback setter)
{
   sGetClipboardText = getter;
   sSetClipboardText = setter;
}

void
updateInput()
{
   auto &io = ImGui::GetIO();
   io.MousePos = ImVec2(sMousePosX, sMousePosY);

   for (int i = 0; i < 3; i++) {
      io.MouseDown[i] = sMouseClicked[i] || sMousePressed[i];
      sMouseClicked[i] = false;
   }

   io.MouseWheel = sMouseWheel;
   sMouseWheel = 0.0;
}

} // namespace ui

} // namespace debugger
