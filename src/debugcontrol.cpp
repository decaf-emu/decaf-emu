#include "debugcontrol.h"
#include "debugmsg.h"
#include "debugger.h"
#include "cpu/cpu.h"

DebugControl
gDebugControl;

DebugControl::DebugControl()
{
   mShouldBePaused = false;
   mWaitingForStep = -1;
}

void
DebugControl::pauseAll()
{
   std::unique_lock<std::mutex> lock{ mMutex };
   mShouldBePaused = true;

   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }
}

void
DebugControl::resumeAll()
{
   std::unique_lock<std::mutex> lock{ mMutex };

   mShouldBePaused = false;

   mReleaseCond.notify_all();
}

void
DebugControl::stepCore(uint32_t coreId)
{
   std::unique_lock<std::mutex> lock{ mMutex };

   mWaitingForStep = coreId;
   mShouldBePaused = false;

   mReleaseCond.notify_all();
}

void
DebugControl::handleDbgBreakInterrupt()
{
   std::unique_lock<std::mutex> lock{ mMutex };
   auto coreId = cpu::this_core::id();

   // If we hit a breakpoint, we should signal all cores to stop
   for (auto i = 0; i < 3; ++i) {
      cpu::interrupt(i, cpu::DBGBREAK_INTERRUPT);
   }

   if (mWaitingForStep == coreId) {
      mWaitingForStep = -1;
   }

   auto msg = new DebugMessageCorePaused();
   msg->coreId = coreId;
   msg->state = cpu::this_core::state();
   msg->wasInitiator = !mShouldBePaused;
   gDebugger.notify(msg);

   mShouldBePaused = true;
   mWaitCond.notify_all();

   while (mShouldBePaused) {
      mReleaseCond.wait(lock);
   }

   // Clear any additional DBGBREAK's that occured
   cpu::this_core::clearInterrupt(cpu::DBGBREAK_INTERRUPT);

   // If we want to single-step this core, we need to interrupt
   // We rely on the fact that our cpu emulator only processes a
   // single interrupt handler run per instruction.
   if (mWaitingForStep == coreId) {
      cpu::interrupt(coreId, cpu::DBGBREAK_INTERRUPT);
   }
}
