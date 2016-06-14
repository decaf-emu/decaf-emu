#include <atomic>
#include "common/emuassert.h"
#include "common/log.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include "debugger/debugger_branchcalc.h"
#include "modules/coreinit/coreinit_internal_loader.h"

namespace debugger
{

static bool
sIsInitialised;

static std::mutex
sMutex;

static std::condition_variable
sPauseReleaseCond;

static std::atomic<uint32_t>
sIsPausing;

static std::atomic_bool
sIsPaused;

static cpu::Core *
sCorePauseState[3];

static uint32_t
sPauseInitiatorCoreId;

void initialise()
{
   sIsInitialised = true;
}

bool isEnabled()
{
   return sIsInitialised;
}

bool isPaused()
{
   return sIsPaused.load();
}

void pauseAll()
{
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }
}

void resumeAll()
{
   auto oldState = sIsPaused.exchange(false);
   emuassert(oldState);
   sPauseReleaseCond.notify_all();
}

static void stepCore(uint32_t coreId, bool stepOver)
{
   emuassert(sIsPaused.load());

   const cpu::CoreRegs *state = sCorePauseState[coreId];
   uint32_t nextInstr = calculateNextInstr(state, stepOver);
   cpu::addBreakpoint(nextInstr, cpu::SYSTEM_BPFLAG);

   resumeAll();
}

void stepCoreInto(uint32_t coreId)
{
   stepCore(coreId, false);
}

void stepCoreOver(uint32_t coreId)
{
   stepCore(coreId, true);
}

void handlePreLaunch()
{
   // If we are not initialised, we should ignore the PreLaunch event
   if (!sIsInitialised) {
      return;
   }

   auto appModule = coreinit::internal::getUserModule();

   auto userPreinit = appModule->findExport("__preinit_user");
   if (userPreinit) {
      cpu::addBreakpoint(userPreinit, cpu::SYSTEM_BPFLAG);
   }

   auto start = appModule->entryPoint;
   cpu::addBreakpoint(start, cpu::SYSTEM_BPFLAG);
}

void handleDbgBreakInterrupt()
{
   // If we are not initialised, we should ignore DbgBreaks
   if (!sIsInitialised) {
      return;
   }

   std::unique_lock<std::mutex> lock(sMutex);
   auto coreId = cpu::this_core::id();

   // If we hit a breakpoint, we should signal all cores to stop
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }

   // Check to see if we were the last core to join on the fun
   auto coreBit = 1 << coreId;
   auto isPausing = sIsPausing.fetch_or(coreBit);
   if ((isPausing | coreBit) == (1 | 2 | 4)) {
      // This was the last core to join.
      sIsPaused.store(true);
      sIsPausing.store(0);
   }

   // Spin around the release condition while we are paused
   while (sIsPaused.load()) {
      sPauseReleaseCond.wait(lock);
   }

   // Clear any additional DbgBreaks that occured
   cpu::this_core::clearInterrupt(cpu::DBGBREAK_INTERRUPT);
}

} // namespace debugger
