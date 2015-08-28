#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include "systemtypes.h"
#include "modules/coreinit/coreinit_core.h"

class Debugger
{
public:
   Debugger();

   void addBreakpoint(uint32_t addr);
   void removeBreakpoint(uint32_t addr);

   void maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreIdx);

protected:
   std::atomic_uint mNumBreakpoints;
   std::vector<uint32_t> mBreakpoints;
   std::mutex mMutex;
   bool mCoreStopped[CoreCount];
   ThreadState * mStates[CoreCount];
   std::atomic_bool mWaitingForCores;
   std::condition_variable mWaitCond;
   std::condition_variable mReleaseCond;

};

extern Debugger gDebugger;
