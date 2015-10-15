#pragma once
#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <Windows.h>
#include "modules/coreinit/coreinit_mutex.h"
#include "ppc.h"

struct OSContext;
struct OSThread;

struct Fiber
{
   Fiber()
   {
      handle = CreateFiber(0, &Fiber::fiberEntryPoint, this);
   }

   ~Fiber()
   {
      DeleteFiber(handle);
   }

   static void __stdcall fiberEntryPoint(void *param);

   uint32_t coreID;
   void *handle = nullptr;
   void *parentFiber = nullptr;
   OSThread *thread = nullptr;
   ThreadState state;
};

struct Core
{
   Core(uint32_t id) :
      id(id)
   {
      nextInterrupt = std::chrono::time_point<std::chrono::system_clock>::max();
   }

   uint32_t id;
   Fiber *currentFiber = nullptr;
   Fiber *interruptedFiber = nullptr;
   Fiber *interruptHandlerFiber = nullptr;
   void *primaryFiber = nullptr;
   std::thread thread;
   std::atomic<bool> interrupt = false;
   std::chrono::system_clock::time_point nextInterrupt;
   std::vector<Fiber *> mFiberDeleteList;
};

class Processor
{
public:
   Processor(size_t cores);

   // Processor
   void start();
   void join();

   // Debugger Helper
   void wakeAllCores();

   // Fiber
   Fiber *createFiber();
   void destroyFiber(Fiber *fiber);

   void queue(Fiber *fiber);
   void reschedule(bool hasSchedulerLock, bool yield = false);
   void yield();
   void exit();

   Fiber *getCurrentFiber();

   // Interrupts
   void handleInterrupt();
   void finishInterrupt();
   void waitFirstInterrupt();

   OSContext *getInterruptContext();

   void setInterrupt(uint32_t core);
   void setInterruptTimer(uint32_t core, std::chrono::time_point<std::chrono::system_clock> when);

   // Core
   uint32_t getCoreID();
   uint32_t getCoreCount();

   const std::vector<Core *> getCoreList() const {
      return mCores;
   }
   const std::vector<Fiber *> getFiberList() const {
      return mFiberList;
   }

protected:
   friend Core;
   friend Fiber;

   void timerEntryPoint();
   void coreEntryPoint(Core *core);
   void fiberEntryPoint(Fiber *fiber);

   Fiber *createFiberNoLock();
   void destroyFiberNoLock(Fiber *fiber);
   Fiber *peekNextFiberNoLock(uint32_t core);
   void queueNoLock(Fiber *fiber);

private:
   std::atomic<bool> mRunning;
   std::vector<Core*> mCores;
   std::mutex mMutex;
   std::condition_variable mCondition;
   std::vector<Fiber *> mFiberQueue;
   std::vector<Fiber *> mFiberList;
   std::thread mTimerThread;
   std::mutex mTimerMutex;
   std::condition_variable mTimerCondition;
};

extern Processor
gProcessor;
