#include "cpu.h"
#include "cpu_alarm.h"
#include "cpu_config.h"
#include "cpu_host_exception.h"
#include "cpu_internal.h"
#include "espresso/espresso_instructionset.h"
#include "interpreter/interpreter.h"
#include "jit/jit.h"
#include "jit/binrec/jit_binrec.h"
#include "mem.h"
#include "mmu.h"

#include <cfenv>
#include <chrono>
#include <common/decaf_assert.h>
#include <common/platform_thread.h>
#include <memory>

namespace cpu
{

std::chrono::time_point<std::chrono::steady_clock>
sStartupTime;

static EntrypointHandler
sCoreEntryPointHandler;

BranchTraceHandler
gBranchTraceHandler;

static bool
sJitEnabled = false;

static std::array<std::unique_ptr<Core>, 3>
sCores { };

static thread_local uint32_t
tCurrentCoreId = InvalidCoreId;

static thread_local cpu::Core *
tCurrentCore = nullptr;

void
initialise()
{
   auto settings = config();
   sJitEnabled = settings->jit.enabled;

   // Initalise cpu!
   initialiseMemory();
   espresso::initialiseInstructionSet();
   interpreter::initialise();

   if (sJitEnabled) {
      auto backend = new jit::BinrecBackend {
         settings->jit.codeCacheSizeMB * 1024 * 1024,
         settings->jit.dataCacheSizeMB * 1024 * 1024
      };
      backend->setOptFlags(settings->jit.optimisationFlags);
      backend->setVerifyEnabled(settings->jit.verify, settings->jit.verifyAddress);
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
addJitReadOnlyRange(virt_addr address,
                    uint32_t size)
{
   jit::addReadOnlyRange(static_cast<uint32_t>(address), size);
}

void
coreEntryPoint(Core *core)
{
   tCurrentCoreId = core->id;
   tCurrentCore = core;
   sCoreEntryPointHandler(core);
}

void
start()
{
   internal::installHostExceptionHandler();

   for (auto i = 0u; i < sCores.size(); ++i) {
      auto core = jit::initialiseCore(i);
      if (!core) {
         core = new Core {};
         core->id = i;
      }

      sCores[i] = std::unique_ptr<Core> { core };
      core->thread = std::thread { coreEntryPoint, core };
      core->next_alarm = std::chrono::steady_clock::time_point::max();

      static const std::string coreNames[] = { "Core #0", "Core #1", "Core #2" };
      platform::setThreadName(&core->thread, coreNames[i]);
   }

   internal::startAlarmThread();
}

void
join()
{
   for (auto &core : sCores) {
      if (core && core->thread.joinable()) {
         core->thread.join();
         core.reset();
      }
   }

   internal::joinAlarmThread();
}

void
halt()
{
   for (auto i = 0; i < 3; ++i) {
      interrupt(i, SRESET_INTERRUPT);
   }

   internal::stopAlarmThread();
}

Core *
getCore(int index)
{
   return sCores[index].get();
}

void
setCoreEntrypointHandler(EntrypointHandler handler)
{
   sCoreEntryPointHandler = handler;
}

void
setSegfaultHandler(SegfaultHandler handler)
{
   internal::setUserSegfaultHandler(handler);
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

uint32_t
id()
{
   return tCurrentCoreId;
}

void
resume()
{
   if (sJitEnabled) {
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
