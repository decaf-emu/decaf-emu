#include "decaf_debug_api.h"

#include "debug_api_controller.h"
#include "debugger/debugger.h"
#include "debugger/debugger_analysis.h"
#include "debugger/debugger_branchcalc.h"
#include "decaf_config.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <libcpu/cpu.h>
#include <libcpu/cpu_breakpoints.h>
#include <mutex>

namespace decaf::debug
{

struct Controller
{
   bool enabled = false;

   //! Whether we are currently paused.
   std::atomic_bool isPaused;

   //! Used to synchronise cores across a pause.
   std::mutex pauseMutex;

   //! Used to synchronise cores across a pause.
   std::condition_variable pauseReleaseCond;

   //! The context running on each core at the time of a pause.
   std::array<cpu::Core *, 3> pausedContexts;

   //! Which core initiated the pause by sending a DbgBreak interrupt.
   int pauseInitiator = -1;

   //! Which cores are trying to pause.
   std::atomic<unsigned> coresPausing;

   //! Which cores are trying to resume.
   std::atomic<unsigned> coresResuming;

   //! Public copy of pausedContexts
   std::array<CpuContext, 3> contexts;

   bool entryPointFound = false;
} sController;

static bool
copyPauseContext(int core)
{
   auto pauseContext = sController.pausedContexts[core];
   if (!pauseContext) {
      return false;
   }

   auto &context = sController.contexts[core];
   context.cia = pauseContext->cia;
   context.nia = pauseContext->nia;

   for (auto i = 0u; i < 32; ++i) {
      context.gpr[i] = pauseContext->gpr[i];
      context.fpr[i] = pauseContext->fpr[i].paired0;
      context.ps1[i] = pauseContext->fpr[i].paired1;
   }

   context.cr = pauseContext->cr.value;
   context.xer = pauseContext->xer.value;
   context.lr = pauseContext->lr;
   context.ctr = pauseContext->ctr;
   context.fpscr = pauseContext->fpscr.value;
   context.pvr = pauseContext->pvr.value;
   context.msr = pauseContext->msr.value;

   for (auto i = 0u; i < 16; ++i) {
      context.sr[i] = pauseContext->sr[i];
   }

   for (auto i = 0u; i < 8; ++i) {
      context.gqr[i] = pauseContext->gqr[i].value;
   }

   context.dar = pauseContext->dar;
   context.dsisr = pauseContext->dsisr;
   context.srr0 = pauseContext->srr0;
   return true;
}

bool
ready()
{
   return sController.entryPointFound;
}

bool
pause()
{
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }

   return true;
}

bool
resume()
{
   if (sController.isPaused.exchange(false)) {
      sController.pausedContexts.fill(nullptr);
      sController.pauseReleaseCond.notify_all();
   }

   return true;
}

bool
isPaused()
{
   return sController.isPaused.load();
}

int
getPauseInitiatorCoreId()
{
   return sController.pauseInitiator;
}

const CpuContext *
getPausedContext(int core)
{
   if (!isPaused() || core < 0 || core > 2) {
      return nullptr;
   }

   return &sController.contexts[core];
}

static uint32_t
calculateNextInstr(const cpu::CoreRegs *state, bool stepOver)
{
   auto instr = mem::read<espresso::Instruction>(state->nia);
   auto data = espresso::decodeInstruction(instr);

   if (data && debugger::analysis::isBranchInstr(data)) {
      auto meta = debugger::analysis::getBranchMeta(state->nia,
                                                    instr, data,
                                                    state->ctr,
                                                    state->cr.value,
                                                    state->lr);

      if (meta.isCall && stepOver) {
         // This is a call and we are stepping over...
         return state->nia + 4;
      }

      if (meta.conditionSatisfied) {
         return meta.target;
      } else {
         return state->nia + 4;
      }
   } else {
      // This is not a branch instruction
      return state->nia + 4;
   }
}

bool
stepInto(int core)
{
   if (core < 0 || core > 2) {
      return false;
   }

   auto next = calculateNextInstr(sController.pausedContexts[core], false);
   cpu::addBreakpoint(next, cpu::Breakpoint::SingleFire);
   resume();
   return true;
}

bool
stepOver(int core)
{
   if (core < 0 || core > 2) {
      return false;
   }

   auto next = calculateNextInstr(sController.pausedContexts[core], true);
   cpu::addBreakpoint(next, cpu::Breakpoint::SingleFire);
   resume();
   return true;
}

bool
hasBreakpoint(VirtualAddress address)
{
   return cpu::hasBreakpoint(address);
}

bool
addBreakpoint(VirtualAddress address)
{
   cpu::addBreakpoint(address, cpu::Breakpoint::MultiFire);
   return true;
}

bool
removeBreakpoint(VirtualAddress address)
{
   cpu::removeBreakpoint(address);
   return true;
}

void
handleDebugBreakInterrupt()
{
   static constexpr unsigned NoCores = 0;
   static constexpr unsigned AllCores = (1 << 0) | (1 << 1) | (1 << 2);

   std::unique_lock<std::mutex> lock { sController.pauseMutex };
   auto coreId = cpu::this_core::id();
   sController.pausedContexts[coreId] = cpu::this_core::state();
   copyPauseContext(coreId);

   // Check to see if we were the last core to join on the fun
   auto coreBit = 1 << coreId;
   auto coresPausing = sController.coresPausing.fetch_or(coreBit);

   if (coresPausing == NoCores) {
      // This is the first core to hit a breakpoint
      sController.pauseInitiator = coreId;

      // Signal the rest of the cores to stop
      for (auto i = 0; i < 3; ++i) {
         cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
      }
   }

   if ((coresPausing | coreBit) == AllCores) {
      // All cores are now paused!
      sController.isPaused.store(true);
      sController.coresPausing.store(0);
      sController.coresResuming.store(0);
   }

   // Spin around the release condition while we are paused
   while (sController.coresPausing.load() || sController.isPaused.load()) {
      sController.pauseReleaseCond.wait(lock);
   }

   // Clear any additional debug interrupts that occured
   cpu::this_core::clearInterrupt(cpu::DBGBREAK_INTERRUPT);

   if ((sController.coresResuming.fetch_or(coreBit) | coreBit) == AllCores) {
      // This is the final core to resume, wake up the other cores
      sController.pauseReleaseCond.notify_all();
   } else {
      // Wait until all cores are ready to resume
      while ((sController.coresResuming.load() | coreBit) != AllCores) {
         sController.pauseReleaseCond.wait(lock);
      }
   }
}


void
notifyEntry(uint32_t preinit, uint32_t entry)
{
   if (config::debugger::break_on_entry) {
      if (preinit) {
         addBreakpoint(preinit);
      }

      if (entry) {
         addBreakpoint(entry);
      }
   }

   sController.entryPointFound = true;
}

} // namespace decaf::debug
