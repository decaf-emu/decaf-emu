#include "decaf_config.h"
#include "debugger/debugger_branchcalc.h"
#include "debugger/debugger_ui_internal.h"
#include "common/decaf_assert.h"
#include "common/log.h"
#include "kernel/kernel.h"
#include "kernel/kernel_loader.h"
#include "libcpu/cpu.h"
#include "libcpu/mem.h"
#include <atomic>
#include <condition_variable>

namespace debugger
{

static std::mutex
sMutex;

static std::condition_variable
sPauseReleaseCond;

static std::atomic<uint32_t>
sIsPausing;

static std::atomic<uint32_t>
sIsResuming;

static std::atomic_bool
sIsPaused;

static cpu::Core *
sCorePauseState[3];

bool
enabled()
{
   return decaf::config::debugger::enabled;
}

bool
paused()
{
   return sIsPaused.load();
}

cpu::Core *
getPausedCoreState(uint32_t coreId)
{
   decaf_check(paused());
   return sCorePauseState[coreId];
}

void
pauseAll()
{
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }
}

void
resumeAll()
{
   auto oldState = sIsPaused.exchange(false);
   decaf_check(oldState);
   for (auto i = 0; i < 3; ++i) {
      sCorePauseState[i] = nullptr;
   }
   sPauseReleaseCond.notify_all();
}

static void
stepCore(uint32_t coreId, bool stepOver)
{
   decaf_check(sIsPaused.load());

   const cpu::CoreRegs *state = sCorePauseState[coreId];
   uint32_t nextInstr = calculateNextInstr(state, stepOver);
   cpu::addBreakpoint(nextInstr, cpu::SYSTEM_BPFLAG);

   resumeAll();
}

void
stepCoreInto(uint32_t coreId)
{
   stepCore(coreId, false);
}

void
stepCoreOver(uint32_t coreId)
{
   stepCore(coreId, true);
}

void
handlePreLaunch()
{
   // Do not add entry breakpoints if debugger is disabled
   if (!decaf::config::debugger::enabled) {
      return;
   }

   // Do not add entry breakpoints if it is disabled
   if (!decaf::config::debugger::break_on_entry) {
      return;
   }

   auto appModule = kernel::getUserModule();
   auto userPreinit = appModule->findExport("__preinit_user");

   if (userPreinit) {
      cpu::addBreakpoint(userPreinit, cpu::SYSTEM_BPFLAG);
   }

   auto start = appModule->entryPoint;
   cpu::addBreakpoint(start, cpu::SYSTEM_BPFLAG);
}

void
handleDbgBreakInterrupt()
{
   // If we are not initialised, we should ignore DbgBreaks
   if (!decaf::config::debugger::enabled) {
      return;
   }

   std::unique_lock<std::mutex> lock(sMutex);
   auto coreId = cpu::this_core::id();

   // If we hit a breakpoint, we should signal all cores to stop
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }

   // Store our core state before we flip isPaused
   sCorePauseState[coreId] = cpu::this_core::state();

   // Check where we stand in the overall order of things
   auto coreBit = 1 << coreId;
   auto isPausing = sIsPausing.fetch_or(coreBit);

   // If this is the core that triggered the interrupt, select it for debugging
   if (isPausing == 0) {
      ui::setActiveCore(coreId);
   }

   // If this is the last core to join, we're now fully paused
   if ((isPausing | coreBit) == (1 | 2 | 4)) {
      sIsPaused.store(true);
      sIsPausing.store(0);
      sIsResuming.store(0);
   }

   // Spin around the release condition while we are paused
   while (sIsPausing.load() || sIsPaused.load()) {
      sPauseReleaseCond.wait(lock);
   }

   // Clear any additional DbgBreaks that occured
   cpu::this_core::clearInterrupt(cpu::DBGBREAK_INTERRUPT);

   // Everyone needs to leave at once in case new breakpoints occur.
   if ((sIsResuming.fetch_or(coreBit) | coreBit) == (1 | 2 | 4)) {
      sPauseReleaseCond.notify_all();
   } else {
      while ((sIsResuming.load() | coreBit) != (1 | 2 | 4)) {
         sPauseReleaseCond.wait(lock);
      }
   }
}

} // namespace debugger
