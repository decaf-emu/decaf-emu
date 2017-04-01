#include "debugger.h"
#include "debugger_analysis.h"
#include "debugger_ui.h"
#include "debugger_ui_internal.h"
#include "decaf_config.h"
#include "gpu/pm4_capture.h"
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

static auto
sDebugViewsVisible = false;

static uint64_t
sResumeCount = 0;

static coreinit::OSThread *
sActiveThread = nullptr;

static std::map<uint32_t, bool>
sBreakpoints;

static bool
sJitProfilingEnabled = false;

static unsigned int
sJitProfilingMask = 7;

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

uint32_t
getThreadCoreId(coreinit::OSThread *thread)
{
   if (thread == coreinit::internal::getCoreRunningThread(0)) {
      return 0;
   } else if (thread == coreinit::internal::getCoreRunningThread(1)) {
      return 1;
   } else if (thread == coreinit::internal::getCoreRunningThread(2)) {
      return 2;
   }
   return -1;
}

cpu::CoreRegs *
getThreadCoreRegs(coreinit::OSThread *thread)
{
   if (!sIsPaused) {
      return nullptr;
   }

   auto threadCoreId = getThreadCoreId(thread);
   if (threadCoreId != -1) {
      return debugger::getPausedCoreState(threadCoreId);
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

   return thread->context.nia;
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

   if (sActiveThread) {
      DisasmView::displayAddress(getThreadNia(sActiveThread));
      StackView::displayAddress(getThreadStack(sActiveThread));
   }
}

void
handleGamePaused()
{
   // Lets start by trying to use the breakpoint core
   uint32_t pausedCore = getPauseInitiatorCoreId();
   auto pauseActiveThread = coreinit::internal::getCoreRunningThread(pausedCore);

   if (!pauseActiveThread) {
      // Now lets just try to find any running thread.
      pauseActiveThread = coreinit::internal::getCoreRunningThread(1);

      if (!pauseActiveThread) {
         pauseActiveThread = coreinit::internal::getCoreRunningThread(0);

         if (!pauseActiveThread) {
            pauseActiveThread = coreinit::internal::getCoreRunningThread(2);
         }
      }
   }

   if (!pauseActiveThread) {
      // Gezus... Pick the first one...
      pauseActiveThread = coreinit::internal::getFirstActiveThread();
   }

   setActiveThread(pauseActiveThread);
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

bool
isVisible()
{
   return sDebugViewsVisible;
}

void
draw()
{
   static auto firstActivation = true;

   if (!debugger::enabled()) {
      return;
   }

   auto &io = ImGui::GetIO();

   if (debugger::paused() && !sIsPaused) {
      // Just Paused
      sIsPaused = true;
      handleGamePaused();

      // Force the debugger to pop up
      sDebugViewsVisible = true;
   }

   if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::D), false)) {
      sDebugViewsVisible = !sDebugViewsVisible;
   }

   // This is a stupid hack to avoid code duplation everywhere her...
   auto wantsPause = false;
   auto wantsResume = false;
   auto wantsStepOver = false;
   auto wantsStepInto = false;

   if (sDebugViewsVisible) {
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

         if (ImGui::MenuItem("Step Over", "F10", false, sIsPaused && getThreadCoreId(sActiveThread) != -1)) {
            wantsStepOver = true;
         }

         if (ImGui::MenuItem("Step Into", "F11", false, sIsPaused && getThreadCoreId(sActiveThread) != -1)) {
            wantsStepInto = true;
         }

         ImGui::Separator();

         if (ImGui::MenuItem("PM4 Capture Next Frame", nullptr, false, true)) {
            pm4::captureNextFrame();
         }

         ImGui::Separator();

         if (ImGui::MenuItem("Kernel Trace Enabled", nullptr, decaf::config::log::kernel_trace, true)) {
            decaf::config::log::kernel_trace = !decaf::config::log::kernel_trace;
         }

         auto pm4Enable = false;
         auto pm4Status = false;

         switch (pm4::captureState()) {
         case pm4::CaptureState::Disabled:
            pm4Status = false;
            pm4Enable = true;
            break;
         case pm4::CaptureState::Enabled:
            pm4Status = true;
            pm4Enable = true;
            break;
         case pm4::CaptureState::WaitEndNextFrame:
            pm4Status = true;
            pm4Enable = false;
            break;
         case pm4::CaptureState::WaitStartNextFrame:
            pm4Status = true;
            pm4Enable = false;
            break;
         }

         if (ImGui::MenuItem("PM4 Trace Enabled", nullptr, pm4Status, pm4Enable)) {
            if (!pm4Status) {
               pm4::captureStartAtNextSwap();
            } else {
               pm4::captureStopAtNextSwap();
            }
         }

         if (ImGui::MenuItem("GX2 Texture Dump Enabled", nullptr, decaf::config::gx2::dump_textures, true)) {
            decaf::config::gx2::dump_textures = !decaf::config::gx2::dump_textures;
         }

         if (ImGui::MenuItem("GX2 Shader Dump Enabled", nullptr, decaf::config::gx2::dump_shaders, true)) {
            decaf::config::gx2::dump_shaders = !decaf::config::gx2::dump_shaders;
         }

         ImGui::Separator();

         if (ImGui::MenuItem("JIT Profiling Enabled", "CTRL+F1/F2", sJitProfilingEnabled, true)) {
            sJitProfilingEnabled = !sJitProfilingEnabled;
            cpu::setJitProfilingMask(sJitProfilingEnabled ? sJitProfilingMask : 0);
         }

         if (ImGui::MenuItem("Profile Core 0", nullptr, sJitProfilingMask & 1, true)) {
            sJitProfilingMask ^= 1;
            if (sJitProfilingEnabled) {
               cpu::setJitProfilingMask(sJitProfilingMask);
            }
         }

         if (ImGui::MenuItem("Profile Core 1", nullptr, sJitProfilingMask & 2, true)) {
            sJitProfilingMask ^= 2;
            if (sJitProfilingEnabled) {
               cpu::setJitProfilingMask(sJitProfilingMask);
            }
         }

         if (ImGui::MenuItem("Profile Core 2", nullptr, sJitProfilingMask & 4, true)) {
            sJitProfilingMask ^= 4;
            if (sJitProfilingEnabled) {
               cpu::setJitProfilingMask(sJitProfilingMask);
            }
         }

         if (ImGui::MenuItem("Reset JIT Profile Data", "CTRL+F3", false, true)) {
            cpu::resetJitProfileData();
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

         if (ImGui::MenuItem("Voices", "CTRL+P", VoicesView::gIsVisible, true)) {
            VoicesView::gIsVisible = !VoicesView::gIsVisible;
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

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::P), false)) {
         VoicesView::gIsVisible = !VoicesView::gIsVisible;
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F1), false)) {
         cpu::setJitProfilingMask(true);
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F2), false)) {
         cpu::setJitProfilingMask(false);
      }

      if (io.KeyCtrl && ImGui::IsKeyPressed(static_cast<int>(decaf::input::KeyboardKey::F3), false)) {
         cpu::resetJitProfileData();
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

      auto activeThreadCoreId = getThreadCoreId(sActiveThread);

      if (wantsStepOver && sIsPaused && activeThreadCoreId != -1) {
         debugger::stepCoreOver(activeThreadCoreId);
         sIsPaused = false;
         handleGameResumed();
      }

      if (wantsStepInto && sIsPaused && activeThreadCoreId != -1) {
         debugger::stepCoreInto(activeThreadCoreId);
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
      VoicesView::draw();
   }
}

} // namespace ui

} // namespace debugger
