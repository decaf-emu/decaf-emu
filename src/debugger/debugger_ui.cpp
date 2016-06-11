#include "debugger_ui.h"
#include <imgui.h>

using namespace decaf::input;

namespace debugger
{

namespace ui
{

const char *(*sGetClipboardText)() = nullptr;
void(*sSetClipboardText)(const char*) = nullptr;

static bool
sMousePressed[3] = { false, false, false };

static bool
sMouseClicked[3] = { false, false, false };

static float
sMouseWheel = 0.0f;

static float
sMousePosX = 0.0f, sMousePosY = 0.0f;

void initialise()
{
   ImGuiIO& io = ImGui::GetIO();
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
}

void injectMouseButtonInput(int button, MouseAction action)
{
   if (button < 0 || button >= 3) {
      return;
   }

   if (action == MouseAction::Press) {
      sMousePressed[button] = true;
      sMouseClicked[button] = true;
   } else if (action == MouseAction::Release) {
      sMousePressed[button] = false;
   }
}

void injectMousePos(double x, double y)
{
   sMousePosX = static_cast<float>(x);
   sMousePosY = static_cast<float>(y);
}

void injectScrollInput(double xoffset, double yoffset)
{
   sMouseWheel += static_cast<float>(yoffset);
}

void injectKeyInput(KeyboardKey key, KeyboardAction action)
{
   ImGuiIO& io = ImGui::GetIO();
   if (action == KeyboardAction::Press)
      io.KeysDown[static_cast<int>(key)] = true;
   if (action == KeyboardAction::Release)
      io.KeysDown[static_cast<int>(key)] = false;

   io.KeyCtrl = io.KeysDown[static_cast<int>(KeyboardKey::LeftControl)] || 
      io.KeysDown[static_cast<int>(KeyboardKey::RightControl)];
   io.KeyShift = io.KeysDown[static_cast<int>(KeyboardKey::LeftShift)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightShift)];
   io.KeyAlt = io.KeysDown[static_cast<int>(KeyboardKey::LeftAlt)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightAlt)];
   io.KeySuper = io.KeysDown[static_cast<int>(KeyboardKey::LeftSuper)] ||
      io.KeysDown[static_cast<int>(KeyboardKey::RightSuper)];
}

void injectCharInput(unsigned short c)
{
   ImGuiIO &io = ImGui::GetIO();
   io.AddInputCharacter(c);
}

void setClipboardTextCallbacks(const char *(*getter)(), void(*setter)(const char*))
{
   sGetClipboardText = getter;
   sSetClipboardText = setter;
}

void updateInput()
{
   ImGuiIO& io = ImGui::GetIO();

   io.MousePos = ImVec2(sMousePosX, sMousePosY);

   for (int i = 0; i < 3; i++)
   {
      io.MouseDown[i] = sMouseClicked[i] || sMousePressed[i];
      sMouseClicked[i] = false;
   }

   io.MouseWheel = sMouseWheel;
   sMouseWheel = 0.0f;
}

} // namespace ui

} // namespace debugger