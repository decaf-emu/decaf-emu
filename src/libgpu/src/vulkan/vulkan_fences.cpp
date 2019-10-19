#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

SyncWaiter *
Driver::allocateSyncWaiter()
{
   if (!mWaiterPool.empty()) {
      auto syncWaiter = mWaiterPool.back();
      mWaiterPool.pop_back();
      return syncWaiter;
   }

   auto syncWaiter = new SyncWaiter();

   // Allocate a fence
   syncWaiter->fence = mDevice.createFence(vk::FenceCreateInfo());

   // Allocate a command buffer
   vk::CommandBufferAllocateInfo cmdBufferAllocDesc(mCommandPool, vk::CommandBufferLevel::ePrimary, 1);
   syncWaiter->cmdBuffer = mDevice.allocateCommandBuffers(cmdBufferAllocDesc)[0];

   return syncWaiter;
}

void
Driver::releaseSyncWaiter(SyncWaiter *syncWaiter)
{
   // Reset Vulkan state for this buffer resource thing
   mDevice.resetFences({ syncWaiter->fence });
   syncWaiter->cmdBuffer.reset(vk::CommandBufferResetFlags());

   // Reset our local state for this buffer resource thing
   syncWaiter->isCompleted = false;
   syncWaiter->callbacks.clear();
   syncWaiter->stagingBuffers.clear();
   syncWaiter->retileHandles.clear();
   syncWaiter->descriptorPools.clear();
   syncWaiter->occQueryPools.clear();

   // Put this fence back in the pool
   mWaiterPool.push_back(syncWaiter);
}

void
Driver::submitSyncWaiter(SyncWaiter *syncWaiter)
{
   std::unique_lock lock(mFenceMutex);
   mFencesWaiting.push_back(syncWaiter);
   mFencesPending.push_back(syncWaiter);
   mFenceSignal.notify_all();
}

void
Driver::executeSyncWaiter(SyncWaiter *syncWaiter)
{
   for (auto &callback : syncWaiter->callbacks) {
      callback();
   }

   for (auto &buffer : syncWaiter->stagingBuffers) {
      retireStagingBuffer(buffer);
   }

   for (auto &handle : syncWaiter->retileHandles) {
      mGpuRetiler.releaseHandle(handle);
   }

   for (auto &pool : syncWaiter->descriptorPools) {
      retireDescriptorPool(pool);
   }

   for (auto &pool : syncWaiter->occQueryPools) {
      retireOccQueryPool(pool);
   }
}

void
Driver::fenceWaiterThread()
{
   std::unique_lock lock(mFenceMutex);

   while (mRunState == RunState::Running) {
      if (mFencesWaiting.size() == 0) {
         mFenceSignal.wait(lock);
         continue;
      }

      auto waiter = mFencesWaiting.front();
      lock.unlock();

      // Wake up every 100ms to check if we are no longer running
      auto waitTimeInNs = 10000000u;

      if (mDevice.waitForFences(1, &waiter->fence, false, waitTimeInNs) == vk::Result::eSuccess) {
         lock.lock();

         waiter->isCompleted = true;
         mFencesWaiting.pop_front();
         gpu::ringbuffer::wake();
      } else {
         lock.lock();
      }
   }
}

void
Driver::checkSyncFences()
{
   std::unique_lock lock(mFenceMutex);

   while (true) {
      // If there are no pending fences, return immediately
      if (mFencesPending.empty()) {
         break;
      }

      // Grab the oldest pending fence and see if its completed
      auto oldestPending = mFencesPending.front();
      if (!oldestPending->isCompleted) {
         break;
      }
      mFencesPending.pop_front();

      // Perform any actions this sync waiter has queued
      executeSyncWaiter(oldestPending);

      // Release the sync waiter back to our pool
      releaseSyncWaiter(oldestPending);
   }
}

void
Driver::addRetireTask(std::function<void()> fn)
{
   mActiveSyncWaiter->callbacks.push_back(fn);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
