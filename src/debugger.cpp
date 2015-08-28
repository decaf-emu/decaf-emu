#include "debugger.h"
#include "log.h"
#include "processor.h"

Debugger
gDebugger;

Debugger::Debugger()
{
   mWaitingForCores.store(false);
   mNumBreakpoints.store(0);

   for (int i = 0; i < CoreCount; ++i) {
      mCoreStopped[i] = false;
      mStates[i] = nullptr;
   }
}

void
Debugger::addBreakpoint(uint32_t addr)
{
   std::unique_lock<std::mutex> lock{ mMutex };
   mBreakpoints.push_back(addr);
   mNumBreakpoints.store(mBreakpoints.size());
}

void
Debugger::removeBreakpoint(uint32_t addr)
{
   std::unique_lock<std::mutex> lock{ mMutex };
   auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), addr);
   if (bpitr != mBreakpoints.end()) {
      mBreakpoints.erase(bpitr);
   }
   mNumBreakpoints.store(mBreakpoints.size());
}

void
Debugger::maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreId)
{
   while (mWaitingForCores.load()) {
      std::unique_lock<std::mutex> lock{ mMutex };

      mStates[coreId] = state;
      mCoreStopped[coreId] = true;
      mWaitCond.notify_all();

      mReleaseCond.wait(lock);
   }

   bool isBpAddr = false;
   if (mNumBreakpoints.load() > 0) {
      // TODO: Somehow make this lockless...
      std::unique_lock<std::mutex> lockX{ mMutex };
      auto bpitr = std::find(mBreakpoints.begin(), mBreakpoints.end(), addr);
      isBpAddr = bpitr != mBreakpoints.end();
   }

   if (isBpAddr) {
      bool prevVal = mWaitingForCores.exchange(true);
      if (prevVal == true) {
         // Another core pre-empted us breaking, 
         //   execute again to catch the loop above
         maybeBreak(addr, state, coreId);
         return;
      }

      gProcessor.wakeAll();

      std::unique_lock<std::mutex> lock{ mMutex };
      mStates[coreId] = state;
      mCoreStopped[coreId] = true;

      while (true) {
         bool allCoresStopped = true;
         for (int i = 0; i < CoreCount; ++i) {
            allCoresStopped &= mCoreStopped[i];
         }
         if (allCoresStopped) {
            break;
         }

         mWaitCond.wait(lock);
      }

      // Cores are all synchronized here...

      gLog->debug("Hit breakpoint!");

      // Leaving Core synchronization

      for (int i = 0; i < CoreCount; ++i) {
         mCoreStopped[i] = false;
         mStates[i] = nullptr;
      }

      mWaitingForCores.store(false);

      mReleaseCond.notify_all();
   }
}