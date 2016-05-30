#include "debugcontrol.h"
#include "debugmsg.h"
#include "debugger.h"

DebugControl
gDebugControl;

DebugControl::DebugControl()
{
   mWaitingForPause.store(false);
   mWaitingForStep.store(-1);

   for (int i = 0; i < DCCoreCount; ++i) {
      mCorePaused[i] = false;
   }
}

void
DebugControl::pauseAll()
{
   auto prevVal = mWaitingForPause.exchange(true);

   if (prevVal == true) {
      // Someone pre-empted this pause
      return;
   }
}

void
DebugControl::resumeAll()
{
   for (auto i = 0; i < DCCoreCount; ++i) {
      mCorePaused[i] = false;
   }

   mWaitingForPause.store(false);
   mReleaseCond.notify_all();
}

void
DebugControl::stepCore(uint32_t coreId)
{
   mCorePaused[coreId] = false;
   mWaitingForStep.store(coreId);
   mReleaseCond.notify_all();
}

void
DebugControl::waitForAllPaused()
{
   std::unique_lock<std::mutex> lock { mMutex };

   while (true) {
      bool allCoresPaused = true;

      for (auto i = 0; i < DCCoreCount; ++i) {
         allCoresPaused &= mCorePaused[i];
      }

      if (allCoresPaused) {
         break;
      }

      // TODO: FIX THIS (DEBUGCONTROL)
      //gProcessor.wakeAllCores();
      mWaitCond.wait(lock);
   }
}

void
DebugControl::pauseCore(cpu::Core *state, uint32_t coreId)
{
   while (mWaitingForPause.load()) {
      std::unique_lock<std::mutex> lock { mMutex };

      mCorePaused[coreId] = true;
      mWaitCond.notify_all();
      mReleaseCond.wait(lock);

      if (mWaitingForStep.load() == coreId) {
         break;
      }
   }
}

void
DebugControl::preLaunch()
{
   if (!gDebugger.isEnabled()) {
      return;
   }

   pauseAll();
   gDebugger.notify(new DebugMessagePreLaunch());
   pauseCore(nullptr, coreinit::OSGetCoreId());
}

void
DebugControl::maybeBreak(uint32_t addr, cpu::Core *state, uint32_t coreId)
{
   if (!gDebugger.isEnabled()) {
      return;
   }

   if (mWaitingForPause.load()) {
      if (mWaitingForStep.load() == coreId) {
         mWaitingForStep.store(-1);

         auto msg = new DebugMessageCoreStepped();
         msg->coreId = coreId;
         gDebugger.notify(msg);
      }

      pauseCore(state, coreId);
      return;
   }

   auto &bps = gDebugger.getBreakpoints();
   auto itr = bps->find(addr);

   if (itr != bps->end()) {
      pauseAll();

      // Send a message to the debugger before we pause ourself
      auto msg = new DebugMessageBpHit();
      msg->coreId = coreId;
      msg->address = addr;
      msg->userData = itr->second;
      gDebugger.notify(msg);

      pauseCore(state, coreId);
      return;
   }
}
