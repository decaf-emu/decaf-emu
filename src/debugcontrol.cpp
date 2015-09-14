#include "debugcontrol.h"
#include "debugmsg.h"
#include "processor.h"
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
   bool prevVal = mWaitingForPause.exchange(true);
   if (prevVal == true) {
      // Someone pre-empted this pause
      return;
   }
}

void
DebugControl::resumeAll()
{
   for (int i = 0; i < DCCoreCount; ++i) {
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
   std::unique_lock<std::mutex> lock{ mMutex };

   while (true) {
      bool allCoresPaused = true;
      for (int i = 0; i < DCCoreCount; ++i) {
         allCoresPaused &= mCorePaused[i];
      }
      if (allCoresPaused) {
         break;
      }
      
      gProcessor.wakeAll();
      mWaitCond.wait(lock);
   }
}

void
DebugControl::pauseCore(ThreadState *state, uint32_t coreId)
{
   while (mWaitingForPause.load()) {
      std::unique_lock<std::mutex> lock{ mMutex };

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
   pauseCore(nullptr, OSGetCoreId());
}


void
DebugControl::maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreId)
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

   BreakpointList bps = gDebugger.getBreakpoints();
   uint32_t bpUserData = 0;
   bool isBpAddr = false;
   auto bpitr = bps->find(addr);
   if (bpitr != bps->end()) {
      bpUserData = bpitr->second;
      isBpAddr = true;
   }

   if (isBpAddr) {
      pauseAll();

      // Send a message to the debugger before we pause ourself
      auto msg = new DebugMessageBpHit();
      msg->coreId = coreId;
      msg->address = addr;
      msg->userData = bpUserData;
      gDebugger.notify(msg);

      pauseCore(state, coreId);
      return;
   }
}