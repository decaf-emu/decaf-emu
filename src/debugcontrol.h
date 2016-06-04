#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>

// TODO: Need to use CoreCount, but needs to not be in CoreInit...
static const int DCCoreCount = 3;

namespace cpu {
   struct Core;
}

class DebugControl
{
public:
   DebugControl();

   void handleDbgBreakInterrupt();
   void pauseAll();
   void resumeAll();
   void stepCore(uint32_t coreId);

protected:
   std::mutex mMutex;
   std::condition_variable mWaitCond;
   std::condition_variable mReleaseCond;
   bool mShouldBePaused;
   uint32_t mWaitingForStep;
};

extern DebugControl gDebugControl;
