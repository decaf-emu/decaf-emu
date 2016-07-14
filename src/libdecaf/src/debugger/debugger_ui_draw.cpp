#include "debugger.h"
#include "debugger_analysis.h"
#include "debugger_ui.h"
#include "debugger_ui_internal.h"
#include "decaf_config.h"
#include "kernel/kernel_loader.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include <imgui.h>
#include <map>

namespace debugger
{

namespace ui
{

// We store this locally so that we do not end up with isRunning
//  switching whilst we are in the midst of drawing the UI.
static bool
sIsPaused = false;

static uint64_t
sResumeCount = 0;

static uint32_t
sActiveCore = 1;

static coreinit::OSThread *
sActiveThread = nullptr;

static std::map<uint32_t, bool>
sBreakpoints;

bool
isPaused()
{
   return sIsPaused;
}

bool
hasBreakpoint(uint32_t address)
{
   auto bpIter = sBreakpoints.find(address);
   return bpIter != sBreakpoints.end() && bpIter->second;
}

void
toggleBreakpoint(uint32_t address)
{
   if (sBreakpoints[address]) {
      cpu::removeBreakpoint(address, cpu::USER_BPFLAG);
      sBreakpoints[address] = false;
   } else {
      cpu::addBreakpoint(address, cpu::USER_BPFLAG);
      sBreakpoints[address] = true;
   }
}

coreinit::OSThread *
getActiveThread()
{
   return sActiveThread;
}

cpu::CoreRegs *
getThreadCoreRegs(coreinit::OSThread *thread)
{
   if (!sIsPaused) {
      return nullptr;
   }

   if (thread == coreinit::internal::getCoreRunningThread(0)) {
      return debugger::getPausedCoreState(0);
   } else if (thread == coreinit::internal::getCoreRunningThread(1)) {
      return debugger::getPausedCoreState(1);
   } else if (thread == coreinit::internal::getCoreRunningThread(2)) {
      return debugger::getPausedCoreState(2);
   }

   return nullptr;
}

uint32_t
getThreadNia(coreinit::OSThread *thread)
{
   decaf_check(sIsPaused);
   auto coreRegs = getThreadCoreRegs(thread);

   if (coreRegs) {
      return coreRegs->nia;
   }

   return mem::read<uint32_t>(thread->context.gpr[1] + 8);
}

uint32_t getThreadStack(coreinit::OSThread *thread)
{
   decaf_check(sIsPaused);
   auto coreRegs = getThreadCoreRegs(thread);

   if (coreRegs) {
      return coreRegs->gpr[1];
   }

   return thread->context.gpr[1];
}

void
setActiveThread(coreinit::OSThread *thread)
{
   decaf_check(sIsPaused);
   sActiveThread = thread;

   if (sActiveThread == coreinit::internal::getCoreRunningThread(0)) {
      sActiveCore = 0;
   } else if (sActiveThread == coreinit::internal::getCoreRunningThread(1)) {
      sActiveCore = 1;
   } else if (sActiveThread == coreinit::internal::getCoreRunningThread(2)) {
      sActiveCore = 2;
   } else {
      sActiveCore = -1;
   }

   if (sActiveThread) {
      DisasmView::displayAddress(getThreadNia(sActiveThread));
      StackView::displayAddress(getThreadStack(sActiveThread));
   }
}

void
handleGamePaused()
{
   if (!sActiveThread && sActiveCore != -1) {
      // Lets first try to find the thread running on our core.
      sActiveThread = coreinit::internal::getCoreRunningThread(sActiveCore);
   }

   if (!sActiveThread) {
      // Now lets just try to find any running thread.
      sActiveThread = coreinit::internal::getCoreRunningThread(0);

      if (!sActiveThread) {
         sActiveThread = coreinit::internal::getCoreRunningThread(1);

         if (!sActiveThread) {
            sActiveThread = coreinit::internal::getCoreRunningThread(2);
         }
      }
   }

   if (!sActiveThread) {
      // Gezus... Pick the first one...
      sActiveThread = coreinit::internal::getFirstActiveThread();
   }

   setActiveThread(sActiveThread);
}

void
handleGameResumed()
{
   sResumeCount++;
   sActiveThread = nullptr;
}

uint64_t
getResumeCount()
{
   return sResumeCount;
}

void
draw()
{
   static auto firstActivation = true;
   static auto debugViewsVisible = false;

   if (!debugger::enabled()) {
      return;
   }

   auto &io = ImGui::GetIO();

   if (debugger::paused() && !sIsPaused) {
      // Just Paused
      sIsPaused = true;
      handleGamePaused();

      // Force the debugger to pop up
      debugViewsVisible = true;
   }

   if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::D), false)) {
      debugViewsVisible = !debugViewsVisible;
   }

   // This is a stupid hack to avoid code duplation everywhere her...
   auto wantsPause = false;
   auto wantsResume = false;
   auto wantsStepOver = false;
   auto wantsStepInto = false;

   if (debugViewsVisible) {
      auto userModule = kernel::getUserModule();

      if (firstActivation && userModule) {
         // Automatically analyse the primary user module
         for (auto &sec : userModule->sections) {
            if (sec.name.compare(".text") == 0) {
               analysis::analyse(sec.start, sec.end);
               break;
            }
         }

         // Place the views somewhere sane to start in case pausing did not place it somewhere
         if (!sIsPaused) {
            MemView::displayAddress(userModule->entryPoint);
            DisasmView::displayAddress(userModule->entryPoint);
         }

         firstActivation = false;
      }

      ImGui::BeginMainMenuBar();

      if (ImGui::BeginMenu("Debug")) {
         if (ImGui::MenuItem("Pause", nullptr, false, !sIsPaused)) {
            wantsPause = true;
         }

         if (ImGui::MenuItem("Resume", "F5", false, sIsPaused)) {
            wantsResume = true;
         }

         if (ImGui::MenuItem("Step Over", "F10", false, sIsPaused && sActiveCore != -1)) {
            wantsStepOver = true;
         }

         if (ImGui::MenuItem("Step Into", "F11", false, sIsPaused && sActiveCore != -1)) {
            wantsStepInto = true;
         }

         ImGui::Separator();

         if (ImGui::MenuItem("Kernel Trace Enabled", nullptr, decaf::config::log::kernel_trace, true)) {
            decaf::config::log::kernel_trace = !decaf::config::log::kernel_trace;
         }

         if (ImGui::MenuItem("GX2 Texture Dump Enabled", nullptr, decaf::config::gx2::dump_textures, true)) {
            decaf::config::gx2::dump_textures = !decaf::config::gx2::dump_textures;
         }

         if (ImGui::MenuItem("GX2 Shader Dump Enabled", nullptr, decaf::config::gx2::dump_shaders, true)) {
            decaf::config::gx2::dump_shaders = !decaf::config::gx2::dump_shaders;
         }

         ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Windows")) {
         if (ImGui::MenuItem("Memory Map", "CTRL+S", SegView::gIsVisible, true)) {
            SegView::gIsVisible = !SegView::gIsVisible;
         }

         if (ImGui::MenuItem("Threads", "CTRL+T", ThreadView::gIsVisible, true)) {
            ThreadView::gIsVisible = !ThreadView::gIsVisible;
         }

         if (ImGui::MenuItem("Memory", "CTRL+M", MemView::gIsVisible, true)) {
            MemView::gIsVisible  = !MemView::gIsVisible;
         }

         if (ImGui::MenuItem("Disassembly", "CTRL+I", DisasmView::gIsVisible, true)) {
            DisasmView::gIsVisible = !DisasmView::gIsVisible;
         }

         if (ImGui::MenuItem("Registers", "CTRL+R", RegView::gIsVisible, true)) {
            RegView::gIsVisible = !RegView::gIsVisible;
         }

         if (ImGui::MenuItem("Stack", "CTRL+E", StackView::gIsVisible, true)) {
            StackView::gIsVisible = !StackView::gIsVisible;
         }

         if (ImGui::MenuItem("Stats", "CTRL+Q", StatsView::gIsVisible, true)) {
            StatsView::gIsVisible = !StatsView::gIsVisible;
         }

         ImGui::EndMenu();
      }

      ImGui::EndMainMenuBar();

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::S), false)) {
         SegView::gIsVisible = !SegView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::T), false)) {
         ThreadView::gIsVisible = !ThreadView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::M), false)) {
         MemView::gIsVisible = !MemView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::I), false)) {
         DisasmView::gIsVisible = !DisasmView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::R), false)) {
         RegView::gIsVisible = !RegView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::E), false)) {
         StackView::gIsVisible = !StackView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::Q), false)) {
         StatsView::gIsVisible = !StatsView::gIsVisible;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F5), false)) {
         wantsResume = true;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F10), true)) {
         wantsStepOver = true;
      }

      if (sIsPaused && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F11), true)) {
         wantsStepInto = true;
      }

      if (wantsPause && !sIsPaused) {
         debugger::pauseAll();
      }

      if (wantsResume && sIsPaused) {
         debugger::resumeAll();
         sIsPaused = false;
         handleGameResumed();
      }

      if (wantsStepOver && sIsPaused && sActiveCore != -1) {
         debugger::stepCoreOver(sActiveCore);
         sIsPaused = false;
         handleGameResumed();
      }

      if (wantsStepInto && sIsPaused && sActiveCore != -1) {
         debugger::stepCoreInto(sActiveCore);
         sIsPaused = false;
         handleGameResumed();
      }

      InfoView::draw();
      SegView::draw();
      ThreadView::draw();
      MemView::draw();
      DisasmView::draw();
      RegView::draw();
      StackView::draw();
      StatsView::draw();
   }
}

} // namespace ui

} // namespace debugger
