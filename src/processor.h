#pragma once
#include <atomic>
#include <algorithm>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include "cpu/state.h"
#include "modules/coreinit/coreinit_mutex.h"
#include "platform/platform_fiber.h"

namespace coreinit
{
struct OSContext;
struct OSThread;
}


/**
 * Per Wii U thread data
 */
struct Fiber
{
   Fiber()
   {
      handle = platform::createFiber(&Fiber::fiberEntryPoint, this);
   }

   ~Fiber()
   {
      platform::destroyFiber(handle);
   }

   static void fiberEntryPoint(void *param);

   uint32_t coreID = 0;
   platform::Fiber *handle = nullptr;
   coreinit::OSThread *thread = nullptr;
   ThreadState state;
};


/**
 * Per-core data
 */
struct Core
{
   Core(uint32_t id) :
      id(id)
   {
      nextInterrupt = std::chrono::time_point<std::chrono::system_clock>::max();
   }

   uint32_t id;
   uint32_t threadId = 0;
   Fiber *currentFiber = nullptr;
   Fiber *interruptedFiber = nullptr;
   Fiber *interruptHandlerFiber = nullptr;
   platform::Fiber *primaryFiberHandle = nullptr;
   std::thread thread;
   std::chrono::system_clock::time_point nextInterrupt;
   std::vector<Fiber *> fiberDeleteList;
   std::vector<Fiber *> fiberPendingList;
   cpu::CoreState state;
};


/**
 * Thread scheduler for the CPU.
 *
 * Has one thread per CPU core, and a thread for running timer interrupts.
 * Each Wii U thread is ran on a CPU core thread as a fiber.
 */
class Processor
{
public:
   Processor(size_t cores);

   // Processor
   void
   start();

   void
   stop();

   // Debugger Helper
   void
   wakeAllCores();

   // Fiber
   Fiber *
   createFiber();

   void
   queue(Fiber *fiber);

   void
   reschedule(bool hasSchedulerLock,
              bool yield = false);

   void
   yield();

   void
   exit();

   Fiber *
   getCurrentFiber();

   // Interrupts
   void
   handleInterrupt();

   void
   finishInterrupt();

   void
   waitFirstInterrupt();

   coreinit::OSContext *
   getInterruptContext();

   void
   setInterruptTimer(uint32_t core,
                     std::chrono::time_point<std::chrono::system_clock> when);

   platform::Fiber *
   handleAccessViolation(ppcaddr_t address);

   // Core
   uint32_t
   getCoreID();

   uint32_t
   getCoreCount();

   const std::vector<Core *>
   getCoreList() const
   {
      return mCores;
   }

   const std::vector<Fiber *>
   getFiberList() const
   {
      return mFiberList;
   }

protected:
   friend Core;
   friend Fiber;

   void
   timerEntryPoint();

   void
   coreEntryPoint(Core *core);

   void
   fiberEntryPoint(Fiber *fiber);

   static void
   handleInterrupt(cpu::CoreState *core, ThreadState *state);

   Fiber *
   createFiberNoLock();

   Fiber *
   peekNextFiberNoLock(uint32_t core);

   void
   queueNoLock(Fiber *fiber);

private:
   std::atomic<bool> mRunning { false };
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
