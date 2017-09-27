#include "cpu.h"
#include "cpu_config.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "jit/binrec/jit_binrec.h"
#include "mem.h"
#include "mmu.h"

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

uint32_t
gJitVerifyAddress = 0;

static size_t
sJitCodeCacheSize = 1024 * 1024 * 1024;

static size_t
sJitDataCacheSize = 512 * 1024 * 1024;

static std::vector<std::string>
sJitOptFlags;

std::array<Core *, 3>
gCore;

static thread_local cpu::Core *
tCurrentCore = nullptr;

static thread_local uint32_t
sSegfaultAddr = 0;

void
initialise()
{
   // Setup config options
   if (config::jit::enabled) {
      if (config::jit::verify) {
         gJitMode = jit_mode::verify;
         gJitVerifyAddress = config::jit::verify_addr;
      } else {
         gJitMode = jit_mode::enabled;
      }

      sJitCodeCacheSize = config::jit::code_cache_size_mb * 1024 * 1024;
      sJitDataCacheSize = config::jit::data_cache_size_mb * 1024 * 1024;
   } else {
      gJitMode = jit_mode::disabled;
   }

   // Initalise cpu!
   initialiseMemory();
   espresso::initialiseInstructionSet();
   interpreter::initialise();

   if (gJitMode != cpu::jit_mode::disabled) {
      auto backend = new jit::BinrecBackend { sJitCodeCacheSize, sJitDataCacheSize };
      backend->setOptFlags(config::jit::opt_flags);
      jit::setBackend(backend);
   }

   sStartupTime = std::chrono::steady_clock::now();
}

void
clearInstructionCache()
{
   cpu::jit::clearCache(0, 0xFFFFFFFF);
}

void
invalidateInstructionCache(uint32_t address,
                           uint32_t size)
{
   cpu::jit::clearCache(address, size);
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
   auto memBase = getBaseVirtualAddress();
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

   for (auto i = 0u; i < gCore.size(); ++i) {
      auto core = jit::initialiseCore(i);
      if (!core) {
         core = new Core {};
         core->id = i;
      }

      core->thread = std::thread { coreEntryPoint, core };
      core->next_alarm = std::chrono::steady_clock::time_point::max();
      gCore[i] = core;

      static const std::string coreNames[] = { "Core #0", "Core #1", "Core #2" };
      platform::setThreadName(&core->thread, coreNames[i]);
   }

   gTimerThread = std::thread { timerEntryPoint };
   platform::setThreadName(&gTimerThread, "Timer Thread");
}

void
join()
{
   for (auto core : gCore) {
      if (core && core->thread.joinable()) {
         core->thread.join();
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
   auto cpuTicks = TimerDuration { ticks };
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
