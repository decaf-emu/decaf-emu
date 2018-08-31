#include "gpu_ringbuffer.h"

#include <condition_variable>
#include <mutex>
#include <queue>

namespace gpu
{

namespace ringbuffer
{

static std::mutex
sQueueMutex;

static std::condition_variable
sQueueCV;

static std::queue<Item>
sQueue;

static void
appendItem(Item item)
{
   std::unique_lock<std::mutex> lock { sQueueMutex };
   sQueue.push(item);
   sQueueCV.notify_all();
}

void
submit(void *context,
       phys_ptr<uint32_t> buffer,
       uint32_t numWords)
{
   auto item = Item {};
   item.context = context;
   item.buffer = buffer;
   item.numWords = numWords;
   appendItem(item);
}

Item
dequeueItem()
{
   std::unique_lock<std::mutex> lock { sQueueMutex };

   if (!sQueue.size()) {
      return { 0 };
   }

   auto next = sQueue.front();
   sQueue.pop();
   return next;
}

Item
waitForItem()
{
   std::unique_lock<std::mutex> lock { sQueueMutex };

   while (!sQueue.size()) {
      sQueueCV.wait(lock);
   }

   auto next = sQueue.front();
   sQueue.pop();
   return next;
}

void
awaken()
{
   appendItem(Item { 0 });
}

} // namespace ringbuffer

} // namespace gpu
