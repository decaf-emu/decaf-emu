#include "interpreter.h"
#include "processor.h"
#include "ppc.h"
#include "modules/coreinit/coreinit_thread.h"

Processor
gProcessor { 3 };

__declspec(thread) Core *
tCurrentCore = nullptr;

void Fiber::fiberEntryPoint(void *param)
{
   gProcessor.fiberEntryPoint(reinterpret_cast<Fiber*>(param));
}

Processor::Processor(size_t cores)
{
   for (auto i = 0u; i < cores; ++i) {
      mCores.push_back(new Core { i });
   }
}

void Processor::start()
{
   mRunning = true;

   for (auto core : mCores) {
      core->thread = std::thread(std::bind(&Processor::coreEntryPoint, this, core));
   }
}

void Processor::join()
{
   mRunning = false;

   for (auto core : mCores) {
      core->thread.join();
   }
}

uint32_t Processor::join(Fiber *fiber)
{
   std::unique_lock<std::mutex> lock(fiber->mutex);

   while (fiber->thread->state != OSThreadState::Moribund) {
      fiber->condition.wait(lock);
   }

   // TODO: Delete fiber?

   return fiber->state.gpr[3];
}

void Processor::coreEntryPoint(Core *core)
{
   tCurrentCore = core;
   core->primaryFiber = ConvertThreadToFiber(NULL);

   while (mRunning) {
      std::unique_lock<std::mutex> lock(mMutex);
      Fiber *nextFiber = nullptr;

      // Find highest priority fiber with matching affinity
      for (auto fiber : mFiberQueue) {
         if (fiber->thread->attr & (1 << core->id)) {
            nextFiber = fiber;
            break;
         }
      }

      if (!nextFiber) {
         // Wait for a fiber
         mCondition.wait(lock);
      } else {
         // Switch to fiber
         core->currentFiber = nextFiber;
         core->currentFiber->parentFiber = core->primaryFiber;
         core->currentFiber->thread->state = OSThreadState::Running;
         lock.unlock();
         SwitchToFiber(core->currentFiber->handle);
      }
   }
}

void Processor::fiberEntryPoint(Fiber *fiber)
{
   gInterpreter.execute(&fiber->state, fiber->state.cia);
   fiber->thread->exitValue = fiber->state.gpr[3];

   // Exit
   SwitchToFiber(fiber->parentFiber);
}

void Processor::queue(Fiber *fiber)
{
   std::lock_guard<std::mutex> lock(mMutex);

   auto compare =
      [](Fiber *lhs, Fiber *rhs) {
      return lhs->thread->basePriority < rhs->thread->basePriority;
   };

   mFiberQueue.insert(std::upper_bound(mFiberQueue.begin(), mFiberQueue.end(), fiber, compare), fiber);
   mCondition.notify_all();
}

void Processor::reschedule()
{
   auto core = tCurrentCore;
   auto fiber = tCurrentCore->currentFiber;
   fiber->thread->state = OSThreadState::Waiting;
   queue(fiber);
   SwitchToFiber(core->primaryFiber);
}

Fiber *Processor::createFiber()
{
   auto fiber = new Fiber();
   mFiberList.push_back(fiber);
   return fiber;
}

Fiber *Processor::getCurrentFiber()
{
   return tCurrentCore->currentFiber;
}

uint32_t Processor::getCoreID()
{
   return tCurrentCore->id;
}

uint32_t Processor::getCoreCount()
{
   return static_cast<uint32_t>(mCores.size());
}
