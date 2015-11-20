#include "commandqueue.h"
#include "gpu/pm4_buffer.h"
#include "modules/gx2/gx2_event.h"
#include "modules/coreinit/coreinit_time.h"
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
queueUserBuffer(void *buffer, uint32_t bytes)
{
   // TODO: Please no allocate
   // ergggggggggghhhh i hate everything about this
   auto buf = new pm4::Buffer {};
   buf->curSize = bytes / 4;
   buf->maxSize = bytes / 4;
   buf->buffer = reinterpret_cast<uint32_t *>(buffer);
   buf->userBuffer = true;
   queueCommandBuffer(buf);
}


void
queueCommandBuffer(pm4::Buffer *buf)
{
   buf->submitTime = OSGetTime();
   gx2::internal::setLastSubmittedTimestamp(buf->submitTime);
   gQueue.appendBuffer(buf);
}


pm4::Buffer *
unqueueCommandBuffer()
{
   return gQueue.waitForBuffer();
}


void
retireCommandBuffer(pm4::Buffer *buf)
{
   gx2::internal::setRetiredTimestamp(buf->submitTime);

   // TODO: Unlazy this
   if (buf->userBuffer) {
      delete buf->buffer;
      delete buf;
   }
}

} // namespace gpu
