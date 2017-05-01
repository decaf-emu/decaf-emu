#include "decaf_config.h"
#include "decaf_debugger.h"
#include "decaf_input.h"
#include "debugger_analysis.h"
#include "debugger_threadutils.h"
#include "debugger_ui_manager.h"
#include "debugger_ui_window_disassembly.h"
#include "debugger_ui_window_info.h"
#include "debugger_ui_window_memory.h"
#include "debugger_ui_window_registers.h"
#include "debugger_ui_window_segments.h"
#include "debugger_ui_window_stack.h"
#include "debugger_ui_window_stats.h"
#include "debugger_ui_window_threads.h"
#include "debugger_ui_window_voices.h"
#include "debugger_ui_window_performance.h"
#include "kernel/kernel_loader.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/gx2/gx2_internal_pm4cap.h"

#include <libcpu/jit_stats.h>
#include <libgpu/gpu_config.h>
#include <imgui.h>

namespace debugger
{

namespace ui
{

static HotKey
ToggleDebugger = HotKey { KeyboardKey::LeftControl, KeyboardKey::D };

static HotKey
PauseHotKey = HotKey {};

static HotKey
ResumeHotKey = HotKey { KeyboardKey::F5 };

static HotKey
StepOverHotKey = HotKey { KeyboardKey::F10, true };

static HotKey
StepIntoHotKey = HotKey { KeyboardKey::F11, true };

static HotKey
JitProfileHotKey = HotKey { KeyboardKey::LeftControl, KeyboardKey::F1 };

static HotKey
ResetJitProfileHotKey = HotKey { KeyboardKey::LeftControl, KeyboardKey::F2 };

Manager::Manager(DebuggerInterface *debugger) :
   mDebugger(debugger)
{
   mMouseClicked.fill(false);
   mMousePressed.fill(false);
}

void
Manager::addWindow(WindowID id,
                   Window *window,
                   HotKey hotkey)
{
   window->initialise(mDebugger, this);
   mWindows.emplace(id, window);
   mWindowHotKeys.emplace(id, hotkey);
}

Window *
Manager::getWindow(WindowID id)
{
   auto itr = mWindows.find(id);

   if (itr == mWindows.end()) {
      return nullptr;
   } else {
      return itr->second;
   }
}

void Manager::load(const std::string &configPath,
                   ClipboardTextGetCallback getClipboardFn,
                   ClipboardTextSetCallback setClipboardFn)
{
   auto &io = ImGui::GetIO();

   // Set config path
   mConfigPath = configPath;
   io.IniFilename = mConfigPath.c_str();

   // Set clipboard functions
   io.GetClipboardTextFn = getClipboardFn;
   io.SetClipboardTextFn = setClipboardFn;

   // Setup key map
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

   // Load default font
   io.Fonts->AddFontDefault();

   // Load Deja Vu Sans Mono
   static const ImWchar iconsRanges[] = { 0x2500, 0x25FF, 0 };
   auto config = ImFontConfig { };
   config.MergeMode = true;
   config.MergeGlyphCenterV = true;
   auto fontPath = decaf::config::system::resources_path + "/fonts/DejaVuSansMono.ttf";
   io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 13.0f, &config, iconsRanges);

   // Set default syle
   auto &style = ImGui::GetStyle();
   style.Colors[ImGuiCol_WindowBg] = ImVec4 { 0.00f, 0.00f, 0.00f, 0.95f };

   // Setup our windows
   addWindow(WindowID::InfoWindow,
             new InfoWindow { "Info" });

   addWindow(WindowID::DisassemblyWindow,
             new DisassemblyWindow { "Disassembly" },
             { KeyboardKey::LeftControl, KeyboardKey::I });

   addWindow(WindowID::MemoryWindow,
             new MemoryWindow { "Memory" },
             { KeyboardKey::LeftControl, KeyboardKey::M });

   addWindow(WindowID::RegistersWindow,
             new RegistersWindow { "Registers" },
             { KeyboardKey::LeftControl, KeyboardKey::R });

   addWindow(WindowID::SegmentsWindow,
             new SegmentsWindow { "Segments" },
             { KeyboardKey::LeftControl, KeyboardKey::S });

   addWindow(WindowID::StackWindow,
             new StackWindow { "Stack" },
             { KeyboardKey::LeftControl, KeyboardKey::E });

   addWindow(WindowID::StatsWindow,
             new StatsWindow { "Stats" },
             { KeyboardKey::LeftControl, KeyboardKey::Q });

   addWindow(WindowID::ThreadsWindow,
             new ThreadsWindow { "Threads" },
             { KeyboardKey::LeftControl, KeyboardKey::T });

   addWindow(WindowID::VoicesWindow,
             new VoicesWindow { "Voices" },
             { KeyboardKey::LeftControl, KeyboardKey::P });

   addWindow(WindowID::PerformanceWindow,
             PerformanceWindow::create("Performance"),
             { KeyboardKey::LeftControl, KeyboardKey::O });
}

void Manager::draw(unsigned width, unsigned height)
{
   auto &io = ImGui::GetIO();
   updateMouseState();
   checkHotKeys();

   // Update some per-frame state information
   io.DisplaySize = ImVec2 { static_cast<float>(width), static_cast<float>(height) };
   io.DeltaTime = 1.0f / 60.0f;

   // Start the frame
   ImGui::NewFrame();

   // If the PC has moved whilst paused then force our previous state into unpaused.
   auto activeThreadNia = getThreadNia(mDebugger, mActiveThread);

   if (mWasPaused && activeThreadNia != 0 && mLastNia != activeThreadNia) {
      mWasPaused = false;
   }

   if (mDebugger->paused() && !mWasPaused) {
      // Check if we have transitioned to paused.
      onPaused();
      mLastNia = activeThreadNia;
   }

   if (mVisible) {
      if (!mHasBeenActivated) {
         onFirstActivation();
         mHasBeenActivated = true;
      }

      drawMenu();

      for (auto &itr : mWindows) {
         auto window = itr.second;
         auto flags = window->flags();

         if (flags & Window::BringToFront) {
            // Bring window to front
            ImGui::SetNextWindowFocus();
            window->show();

            // Clear the BringToFront flag
            flags = static_cast<Window::Flags>(flags & ~Window::BringToFront);
            window->setFlags(flags);
         }

         if (window->visible()) {
            window->draw();
         }
      }
   }

   if (!mDebugger->paused() && mWasPaused) {
      // Check if we have transitioned to resumed.
      onResumed();
   }

   ImGui::Render();
}

void Manager::onFirstActivation()
{
   auto userModule = kernel::getUserModule();

   // Automatically analyse the primary user module
   for (auto &sec : userModule->sections) {
      if (sec.name.compare(".text") == 0) {
         analysis::analyse(sec.start, sec.end);
         break;
      }
   }

   // Place the views somewhere sane to start in case pausing did not place it somewhere
   if (!mDebugger->paused()) {
      gotoMemoryAddress(userModule->entryPoint);
      gotoDisassemblyAddress(userModule->entryPoint);
   }
}

void Manager::onPaused()
{
   // Try use the thread that the debug interrupt came from.
   auto initiator = mDebugger->getPauseInitiator();
   auto thread = coreinit::internal::getCoreRunningThread(initiator);

   // For some reason if that failed, try pick a running thread.
   if (!thread) {
      thread = coreinit::internal::getCoreRunningThread(1);
   }

   if (!thread) {
      thread = coreinit::internal::getCoreRunningThread(0);
   }

   if (!thread) {
      thread = coreinit::internal::getCoreRunningThread(2);
   }

   // Still no thread??? Try the first active thread in list!
   if (!thread) {
      thread = coreinit::internal::getFirstActiveThread();
   }

   setActiveThread(thread);
   mWasPaused = true;
   mVisible = true;
}

void Manager::onResumed()
{
   mResumeCount++;
   mWasPaused = false;
}

void Manager::checkHotKeys()
{
   if (checkHotKey(ToggleDebugger)) {
      mVisible = !mVisible;
   }

   // Check Window hotkeys
   for (auto &itr : mWindowHotKeys) {
      auto &id = itr.first;
      auto &hotkey = itr.second;

      if (checkHotKey(hotkey)) {
         auto window = getWindow(id);

         if (window->visible()) {
            window->hide();
         } else {
            window->bringToFront();
         }
      }
   }

   // Check individual hotkeys
   if (mDebugger->paused()) {
      if (checkHotKey(ResumeHotKey)) {
         mDebugger->resume();
      } else if (checkHotKey(StepOverHotKey)) {
         auto core = getThreadCoreId(mActiveThread);

         if (core != -1) {
            mDebugger->stepOver(core);
         }
      } else if (checkHotKey(StepIntoHotKey)) {
         auto core = getThreadCoreId(mActiveThread);

         if (core != -1) {
            mDebugger->stepInto(core);
         }
      }
   } else {
      if (checkHotKey(PauseHotKey)) {
         mDebugger->pause();
      }
   }

   if (checkHotKey(JitProfileHotKey)) {
      mJitProfilingEnabled = !mJitProfilingEnabled;
      cpu::jit::setProfilingMask(mJitProfilingEnabled ? mJitProfilingMask : 0);
   }

   if (checkHotKey(ResetJitProfileHotKey)) {
      cpu::jit::resetProfileStats();
   }
}

void Manager::drawMenu()
{
   auto getWindowShortcut = [this](WindowID id) -> const char * {
         auto itr = mWindowHotKeys.find(id);

         if (itr == mWindowHotKeys.end()) {
            return nullptr;
         }

         return itr->second.str.c_str();
      };

   ImGui::BeginMainMenuBar();

   // Draw Debug menu
   if (ImGui::BeginMenu("Debug")) {
      if (ImGui::MenuItem("Pause", PauseHotKey, false, !mDebugger->paused())) {
         mDebugger->pause();
      }

      if (ImGui::MenuItem("Resume", ResumeHotKey, false, mDebugger->paused())) {
         mDebugger->resume();
      }

      if (ImGui::MenuItem("Step Over", StepOverHotKey, false, mDebugger->paused() && getThreadCoreId(mActiveThread) != -1)) {
         mDebugger->stepOver(getThreadCoreId(mActiveThread));
      }

      if (ImGui::MenuItem("Step Into", StepIntoHotKey, false, mDebugger->paused() && getThreadCoreId(mActiveThread) != -1)) {
         mDebugger->stepInto(getThreadCoreId(mActiveThread));
      }

      ImGui::Separator();

      if (ImGui::MenuItem("PM4 Capture Next Frame", nullptr, false, true)) {
         gx2::internal::captureNextFrame();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Kernel Trace Enabled", nullptr, decaf::config::log::kernel_trace, true)) {
         decaf::config::log::kernel_trace = !decaf::config::log::kernel_trace;
      }

      auto pm4Enable = false;
      auto pm4Status = false;

      switch (gx2::internal::captureState()) {
      case gx2::internal::CaptureState::Disabled:
         pm4Status = false;
         pm4Enable = true;
         break;
      case gx2::internal::CaptureState::Enabled:
         pm4Status = true;
         pm4Enable = true;
         break;
      case gx2::internal::CaptureState::WaitEndNextFrame:
         pm4Status = true;
         pm4Enable = false;
         break;
      case gx2::internal::CaptureState::WaitStartNextFrame:
         pm4Status = true;
         pm4Enable = false;
         break;
      }

      if (ImGui::MenuItem("PM4 Trace Enabled", nullptr, pm4Status, pm4Enable)) {
         if (!pm4Status) {
            gx2::internal::captureStartAtNextSwap();
         } else {
            gx2::internal::captureStopAtNextSwap();
         }
      }

      if (ImGui::MenuItem("GX2 Texture Dump Enabled", nullptr, decaf::config::gx2::dump_textures, true)) {
         decaf::config::gx2::dump_textures = !decaf::config::gx2::dump_textures;
      }

      if (ImGui::MenuItem("GX2 Shader Dump Enabled", nullptr, decaf::config::gx2::dump_shaders, true)) {
         decaf::config::gx2::dump_shaders = !decaf::config::gx2::dump_shaders;
      }

      if (ImGui::MenuItem("GPU Shader Dump Enabled", nullptr, gpu::config::dump_shaders, true)) {
         gpu::config::dump_shaders = !gpu::config::dump_shaders;
      }

      ImGui::Separator();

      if (ImGui::MenuItem("JIT Profiling Enabled", JitProfileHotKey, mJitProfilingEnabled, true)) {
         mJitProfilingEnabled = !mJitProfilingEnabled;
         cpu::jit::setProfilingMask(mJitProfilingEnabled ? mJitProfilingMask : 0);
      }

      if (ImGui::MenuItem("Profile Core 0", nullptr, mJitProfilingMask & 1, true)) {
         mJitProfilingMask ^= 1;

         if (mJitProfilingEnabled) {
            cpu::jit::setProfilingMask(mJitProfilingMask);
         }
      }

      if (ImGui::MenuItem("Profile Core 1", nullptr, mJitProfilingMask & 2, true)) {
         mJitProfilingMask ^= 2;

         if (mJitProfilingEnabled) {
            cpu::jit::setProfilingMask(mJitProfilingMask);
         }
      }

      if (ImGui::MenuItem("Profile Core 2", nullptr, mJitProfilingMask & 4, true)) {
         mJitProfilingMask ^= 4;

         if (mJitProfilingEnabled) {
            cpu::jit::setProfilingMask(mJitProfilingMask);
         }
      }

      if (ImGui::MenuItem("Reset JIT Profile Data", ResetJitProfileHotKey, false, true)) {
         cpu::jit::resetProfileStats();
      }

      ImGui::EndMenu();
   }

   // Draw Window menu
   if (ImGui::BeginMenu("Windows")) {
      for (auto &itr : mWindows) {
         auto id = itr.first;
         auto window = itr.second;

         if (window->flags() & Window::AlwaysVisible) {
            continue;
         }

         if (ImGui::MenuItem(window->name().c_str(),
                             getWindowShortcut(id),
                             window->visible(), true)) {
            if (window->visible()) {
               window->hide();
            } else {
               window->show();
            }
         }
      }

      ImGui::EndMenu();
   }

   ImGui::EndMainMenuBar();
}

void
Manager::updateMouseState()
{
   auto &io = ImGui::GetIO();
   io.MousePos = ImVec2 { mMousePosX, mMousePosY };

   for (int i = 0; i < 3; i++) {
      io.MouseDown[i] = mMouseClicked[i] || mMousePressed[i];
      mMouseClicked[i] = false;
   }

   io.MouseWheel = mMouseScrollY;
   mMouseScrollY = 0.0f;
}

bool
Manager::onMouseAction(MouseButton button, MouseAction action)
{
   if (button == MouseButton::Unknown) {
      return false;
   }

   auto id = static_cast<int>(button);

   if (action == MouseAction::Press) {
      mMousePressed[id] = true;
      mMouseClicked[id] = true;
   } else if (action == MouseAction::Release) {
      mMousePressed[id] = false;
   }

   return true;
}

bool
Manager::onMouseMove(float x, float y)
{
   mMousePosX = x;
   mMousePosY = y;
   return true;
}

bool
Manager::onMouseScroll(float x, float y)
{
   mMouseScrollX += x;
   mMouseScrollY += y;
   return true;
}

bool
Manager::onKeyAction(KeyboardKey key, KeyboardAction action)
{
   auto &io = ImGui::GetIO();
   auto idx = static_cast<int>(key);

   if (action == KeyboardAction::Press) {
      io.KeysDown[idx] = true;
   }

   if (action == KeyboardAction::Release) {
      io.KeysDown[idx] = false;
   }

   io.KeyCtrl = io.KeysDown[static_cast<int>(KeyboardKey::LeftControl)]
             || io.KeysDown[static_cast<int>(KeyboardKey::RightControl)];

   io.KeyShift = io.KeysDown[static_cast<int>(KeyboardKey::LeftShift)]
              || io.KeysDown[static_cast<int>(KeyboardKey::RightShift)];

   io.KeyAlt = io.KeysDown[static_cast<int>(KeyboardKey::LeftAlt)]
            || io.KeysDown[static_cast<int>(KeyboardKey::RightAlt)];

   io.KeySuper = io.KeysDown[static_cast<int>(KeyboardKey::LeftSuper)]
              || io.KeysDown[static_cast<int>(KeyboardKey::RightSuper)];

   return true;
}

bool
Manager::onText(const char *text)
{
   auto &io = ImGui::GetIO();
   io.AddInputCharactersUTF8(text);
   return true;
}

bool
Manager::checkHotKey(const HotKey &hk)
{
   auto &io = ImGui::GetIO();

   switch (hk.modifier) {
   case KeyboardKey::LeftControl:
      if (!io.KeyCtrl) {
         return false;
      }
      break;
   case KeyboardKey::RightShift:
   case KeyboardKey::LeftShift:
      if (!io.KeyShift) {
         return false;
      }
      break;
   case KeyboardKey::LeftAlt:
   case KeyboardKey::RightAlt:
      if (!io.KeyAlt) {
         return false;
      }
      break;
   case KeyboardKey::LeftSuper:
   case KeyboardKey::RightSuper:
      if (!io.KeySuper) {
         return false;
      }
      break;
   }

   if (hk.key == KeyboardKey::Unknown) {
      return false;
   }

   return ImGui::IsKeyPressed(static_cast<int>(hk.key), hk.repeat);
}

coreinit::OSThread *
Manager::getActiveThread()
{
   return mActiveThread;
}

void
Manager::setActiveThread(coreinit::OSThread *thread)
{
   mActiveThread = thread;
   gotoStackAddress(getThreadStack(mDebugger, thread));
   gotoDisassemblyAddress(getThreadNia(mDebugger, thread));
}

unsigned
Manager::getResumeCount()
{
   return mResumeCount;
}

void
Manager::gotoDisassemblyAddress(uint32_t address)
{
   auto disassemblyWindow = reinterpret_cast<DisassemblyWindow *>(getWindow(WindowID::DisassemblyWindow));

   if (disassemblyWindow) {
      disassemblyWindow->gotoAddress(address);
      disassemblyWindow->bringToFront();
   }
}

void
Manager::gotoMemoryAddress(uint32_t address)
{
   auto window = reinterpret_cast<MemoryWindow *>(getWindow(WindowID::MemoryWindow));

   if (window) {
      window->gotoAddress(address);
      window->bringToFront();
   }
}

void
Manager::gotoStackAddress(uint32_t address)
{
   auto stackWindow = reinterpret_cast<StackWindow *>(getWindow(WindowID::StackWindow));

   if (stackWindow) {
      stackWindow->gotoAddress(address);
      stackWindow->bringToFront();
   }
}

} // namespace ui

} // namespace debugger
