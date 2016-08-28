#include "gpu_commandqueue.h"
#include "pm4_buffer.h"
#include "pm4_capture.h"
#include "modules/gx2/gx2_event.h"
#include "modules/gx2/gx2_cbpool.h"
#include "modules/coreinit/coreinit_time.h"
#include <condition_variable>
#include <queue>
#include <mutex>

// TODO: LOCK FREE SHIT

namespace gpu
{

class CommandQueue
{
public:
   void appendBuffer(pm4::Buffer *buf)
   {
      std::unique_lock<std::mutex> lock { mQueueMutex };
      mQueue.push(buf);
      mQueueCV.notify_all();
   }

   pm4::Buffer *dequeueBuffer()
   {
      std::unique_lock<std::mutex> lock{ mQueueMutex };

      if (!mQueue.size()) {
         return nullptr;
      }

      auto next = mQueue.front();
      mQueue.pop();
      return next;
   }

   pm4::Buffer *waitForBuffer()
   {
      std::unique_lock<std::mutex> lock { mQueueMutex };

      while (!mQueue.size()) {
         mQueueCV.wait(lock);
      }

      auto next = mQueue.front();
      mQueue.pop();
      return next;
   }

private:
   std::mutex mQueueMutex;
   std::condition_variable mQueueCV;
   std::queue<pm4::Buffer *> mQueue;
};

static CommandQueue
gQueue;

void
awaken()
{
   gQueue.appendBuffer(nullptr);
}

void
queueCommandBuffer(pm4::Buffer *buf)
{
   captureCommandBuffer(buf);
   buf->submitTime = coreinit::OSGetTime();
   gx2::internal::setLastSubmittedTimestamp(buf->submitTime);
   gQueue.appendBuffer(buf);
}


pm4::Buffer *
unqueueCommandBuffer()
{
   return gQueue.waitForBuffer();
}


pm4::Buffer *
tryUnqueueCommandBuffer()
{
   return gQueue.dequeueBuffer();
}

void
retireCommandBuffer(pm4::Buffer *buf)
{
   gx2::internal::setRetiredTimestamp(buf->submitTime);
   gx2::internal::freeCommandBuffer(buf);
}

} // namespace gpu
