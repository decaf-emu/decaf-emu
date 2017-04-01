#include "cpu.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "mem.h"

#include <atomic>
#include <cfenv>
#include <chrono>
#include <common/decaf_assert.h>
#include <common/platform_exception.h>
#include <common/platform_thread.h>
#include <condition_variable>
#include <memory>
#include <vector>

namespace cpu
{

std::atomic_bool
gRunning;

std::chrono::time_point<std::chrono::steady_clock>
sStartupTime;

EntrypointHandler
gCoreEntryPointHandler;

SegfaultHandler
gSegfaultHandler;

IllInstHandler
gIllInstHandler;

BranchTraceHandler
gBranchTraceHandler;

jit_mode
gJitMode = jit_mode::disabled;

static size_t
sJitCacheSize = 0x40000000;

Core
gCore[3];

static thread_local cpu::Core *
tCurrentCore = nullptr;

static thread_local uint32_t
sSegfaultAddr = 0;

void
initialise()
{
   espresso::initialiseInstructionSet();
   cpu::interpreter::initialise();
   cpu::jit::initialise(sJitCacheSize);

   sStartupTime = std::chrono::steady_clock::now();
}

void
setJitMode(jit_mode mode)
{
   gJitMode = mode;
}

void
setJitCacheSize(size_t size)
{
   sJitCacheSize = size;
}

void
setJitOptFlags(const std::vector<std::string> &flags)
{
   jit::setOptFlags(flags);
}

void
addJitReadOnlyRange(ppcaddr_t address, uint32_t size)
{
   jit::addReadOnlyRange(address, size);
}

static void
coreSegfaultEntry()
{
   gSegfaultHandler(sSegfaultAddr);
   decaf_abort("The CPU segfault handler must never return.");
}

static void
coreIllInstEntry()
{
   gIllInstHandler();
   decaf_abort("The CPU illegal instruction handler must never return.");
}

static platform::ExceptionResumeFunc
exceptionHandler(platform::Exception *exception)
{
   // Handle illegal instructions!
   if (exception->type == platform::Exception::InvalidInstruction) {
      return coreIllInstEntry;
   }

   // Only handle AccessViolation exceptions
   if (exception->type != platform::Exception::AccessViolation) {
      return platform::UnhandledException;
   }

   // Only handle exceptions from the CPU cores
   if (this_core::id() >= 0xFF) {
      return platform::UnhandledException;
   }

   // Retreive the exception information
   auto info = reinterpret_cast<platform::AccessViolationException *>(exception);
   auto address = info->address;

   // Only handle exceptions within the memory bounds
   auto memBase = mem::base();
   if (address != 0 && (address < memBase || address >= memBase + 0x100000000)) {
      return platform::UnhandledException;
   }

   sSegfaultAddr = static_cast<uint32_t>(address - memBase);
   return coreSegfaultEntry;
}

void
installExceptionHandler()
{
   static bool handlerInstalled = false;
   if (!handlerInstalled) {
      handlerInstalled = true;
      platform::installExceptionHandler(exceptionHandler);
   }
}

void
coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   gCoreEntryPointHandler();
}

void
start()
{
   installExceptionHandler();

   gRunning.store(true);

   for (auto i = 0; i < 3; ++i) {
      auto &core = gCore[i];
      core.id = i;
      core.thread = std::thread(coreEntryPoint, &core);
      core.next_alarm = std::chrono::steady_clock::time_point::max();

      jit::initCore(&core);

      static const std::string coreNames[] = { "Core #0", "Core #1", "Core #2" };
      platform::setThreadName(&core.thread, coreNames[core.id]);
   }

   gTimerThread = std::thread(timerEntryPoint);
   platform::setThreadName(&gTimerThread, "Timer Thread");
}

void
join()
{
   for (auto i = 0; i < 3; ++i) {
      if (gCore[i].thread.joinable()) {
         gCore[i].thread.join();
      }
   }

   // Mark the CPU as no longer running
   gRunning.store(false);

   // Notify the timer thread that something changed
   gTimerCondition.notify_all();

   // Wait for the timer thread to shut down
   if (gTimerThread.joinable()) {
      gTimerThread.join();
   }
}

void
halt()
{
   for (auto i = 0; i < 3; ++i) {
      interrupt(i, SRESET_INTERRUPT);
   }
}

void
setCoreEntrypointHandler(EntrypointHandler handler)
{
   gCoreEntryPointHandler = handler;
}

void
setSegfaultHandler(SegfaultHandler handler)
{
   gSegfaultHandler = handler;
}

void
setIllInstHandler(IllInstHandler handler)
{
   gIllInstHandler = handler;
}

void
setBranchTraceHandler(BranchTraceHandler handler)
{
   gBranchTraceHandler = handler;
}

std::chrono::steady_clock::time_point
tbToTimePoint(uint64_t ticks)
{
   auto cpuTicks = TimerDuration(ticks);
   auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(cpuTicks);
   return sStartupTime + nanos;
}

uint64_t
Core::tb()
{
   auto now = std::chrono::steady_clock::now();
   auto ticks = std::chrono::duration_cast<TimerDuration>(now - sStartupTime);
   return ticks.count();
}

namespace this_core
{

cpu::Core *
state()
{
   return tCurrentCore;
}

void
resume()
{
   // If we have breakpoints set, we have to fall back to interpreter loop
   // This is because JIT wont check for breakpoints on each instruction.
   // TODO: Make JIT account for breakpoints by inserting breakpoint checks
   // in the appropriate places in the instruction stream.
   if (hasBreakpoints()) {
      interpreter::resume();
      return;
   }

   // Use appropriate jit mode
   if (gJitMode != jit_mode::disabled) {
      jit::resume();
   } else {
      interpreter::resume();
   }
}

void
executeSub()
{
   auto lr = tCurrentCore->lr;
   tCurrentCore->lr = CALLBACK_ADDR;
   resume();
   tCurrentCore->lr = lr;
}

void
updateRoundingMode()
{
   Core *core = tCurrentCore;

   static const int modes[4] = {
      FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD
   };
   fesetround(modes[core->fpscr.rn]);
}

} // namespace this_core

} // namespace cpu
