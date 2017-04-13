#include "debugger_branchcalc.h"
#include "debugger_controller.h"

#include <atomic>
#include <libcpu/cpu.h>
#include <libcpu/cpu_breakpoints.h>
#include <mutex>

namespace debugger
{

static constexpr unsigned
NoCores = 0;

static constexpr unsigned
AllCores = (1 << 0) | (1 << 1) | (1 << 2);

bool Controller::paused()
{
   return mIsPaused.load();
}

cpu::Core *Controller::getPauseContext(unsigned core)
{
   return mPausedContexts[core];
}

unsigned Controller::getPauseInitiator()
{
   return mPauseInitiator;
}

void Controller::pause()
{
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }
}

void Controller::resume()
{
   auto wasPaused = mIsPaused.exchange(false);

   if (wasPaused) {
      mPausedContexts.fill(nullptr);
      mPauseReleaseCond.notify_all();
   }
}

void Controller::stepInto(unsigned core)
{
   auto state = getPauseContext(core);
   auto next = analysis::calculateNextInstr(state, false);
   cpu::addBreakpoint(next, cpu::Breakpoint::SingleFire);
   resume();
}

void Controller::stepOver(unsigned core)
{
   auto state = getPauseContext(core);
   auto next = analysis::calculateNextInstr(state, true);
   cpu::addBreakpoint(next, cpu::Breakpoint::SingleFire);
   resume();
}

bool Controller::hasBreakpoint(uint32_t address)
{
   return cpu::hasBreakpoint(address);
}

void Controller::addBreakpoint(uint32_t address)
{
   cpu::addBreakpoint(address, cpu::Breakpoint::MultiFire);
}

void Controller::removeBreakpoint(uint32_t address)
{
   cpu::removeBreakpoint(address);
}

void Controller::onDebugBreakInterrupt()
{
   std::unique_lock<std::mutex> lock { mPauseMutex };
   auto coreId = cpu::this_core::id();
   mPausedContexts[coreId] = cpu::this_core::state();

   // Check to see if we were the last core to join on the fun
   auto coreBit = 1 << coreId;
   auto coresPausing = mCoresPausing.fetch_or(coreBit);

   if (coresPausing == NoCores) {
      // This is the first core to hit a breakpoint
      mPauseInitiator = coreId;

      // Signal the rest of the cores to stop
      for (auto i = 0; i < 3; ++i) {
         cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
      }
   }

   if ((coresPausing | coreBit) == AllCores) {
      // All cores are now paused!
      mIsPaused.store(true);
      mCoresPausing.store(0);
      mCoresResuming.store(0);
   }

   // Spin around the release condition while we are paused
   while (mCoresPausing.load() || mIsPaused.load()) {
      mPauseReleaseCond.wait(lock);
   }

   // Clear any additional debug interrupts that occured
   cpu::this_core::clearInterrupt(cpu::DBGBREAK_INTERRUPT);

   if ((mCoresResuming.fetch_or(coreBit) | coreBit) == AllCores) {
      // This is the final core to resume, wake up the other cores
      mPauseReleaseCond.notify_all();
   } else {
      // Wait until all cores are ready to resume
      while ((mCoresResuming.load() | coreBit) != AllCores) {
         mPauseReleaseCond.wait(lock);
      }
   }
}

} // namespace debugger
