#pragma once
#include <mutex>
#include <atomic>
#include "systemtypes.h"

// TODO: Need to use CoreCount, but needs to not be in CoreInit...
static const int DCCoreCount = 3;

class DebugControl
{
public:
   DebugControl();

   void preLaunch();
   void maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreIdx);

   void pauseCore(ThreadState *state, uint32_t coreId);
   void pauseAll();
   void resumeAll();
   void waitForAllPaused();

protected:
   std::mutex mMutex;
   bool mCorePaused[DCCoreCount];
   std::atomic_bool mWaitingForPause;
   std::condition_variable mWaitCond;
   std::condition_variable mReleaseCond;

};

extern DebugControl gDebugControl;
