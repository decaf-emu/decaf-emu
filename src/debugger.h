#pragma once
#include <map>
#include <mutex>
#include <atomic>
#include <queue>
#include "systemtypes.h"
#include "modules/coreinit/coreinit_core.h"

class DebugPacket;

class Debugger
{
public:
   Debugger();

   void initialise();

   bool hasDebugger() const;

   void preLaunch();
   void maybeBreak(uint32_t addr, ThreadState *state, uint32_t coreIdx);

protected:
   void addMessage(DebugPacket *msg);
   void debugThread();

   void pauseCore(ThreadState *state, uint32_t coreId);

   void addBreakpoint(uint32_t addr, uint32_t userData);
   void removeBreakpoint(uint32_t addr);

   void pauseAll();
   void resumeAll();
   void waitForAllPaused();

   bool mConnected;
   std::atomic_size_t mNumBreakpoints;
   std::map<uint32_t, uint32_t> mBreakpoints;
   std::mutex mMutex;
   bool mCoreStopped[CoreCount];
   ThreadState * mStates[CoreCount];
   std::atomic_bool mWaitingForPause;
   std::condition_variable mWaitCond;
   std::condition_variable mReleaseCond;
   std::thread mDebuggerThread;
   std::queue<DebugPacket*> mMsgQueue;
   std::mutex mMsgLock;
   std::condition_variable mMsgCond;

};

extern Debugger gDebugger;
