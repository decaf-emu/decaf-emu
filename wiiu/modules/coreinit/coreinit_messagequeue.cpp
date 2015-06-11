#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_messagequeue.h"
#include "util.h"

p32<WMessageQueue>
gSystemMessageQueue;

p32<OSMessage>
gSystemMessageArray;

void
OSInitMessageQueue(WMessageQueue *queue, p32<OSMessage> messages, int32_t size)
{
   OSInitMessageQueueEx(queue, messages, size, nullptr);
}

void
OSInitMessageQueueEx(WMessageQueue *queue, p32<OSMessage> messages, int32_t size, char *name)
{
   new (queue) WMessageQueue;
   queue->tag = WMessageQueue::Tag;
   queue->name = name;
   queue->first = 0;
   queue->count = 0;
   queue->size = size;
   queue->messages = messages;
   queue->mutex = std::make_unique<std::mutex>();
   queue->waitRead = std::make_unique<std::condition_variable>();
   queue->waitSend = std::make_unique<std::condition_variable>();
}

// Insert at back of message queue
BOOL
OSSendMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags flags)
{
   std::unique_lock<std::mutex> lock { *queue->mutex };

   if (!testFlag(flags, OSMessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      queue->waitSend->wait(lock);
   }

   auto index = (queue->first + queue->count) % queue->size;
   auto dst = make_p32<OSMessage>(queue->messages.value + index * sizeof(OSMessage));

   // Copy into message array
   memcpy(dst, message, sizeof(OSMessage));
   queue->count++;

   // Wake up anyone waiting for message to read
   lock.unlock();
   queue->waitRead->notify_one();
   return TRUE;
}

// Insert at front of message queue
BOOL
OSJamMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags flags)
{
   std::unique_lock<std::mutex> lock { *queue->mutex };
   uint32_t index;

   if (!testFlag(flags, OSMessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      queue->waitSend->wait(lock);
   }

   // Get index before front
   if (queue->first == 0) {
      index = queue->size - 1;
   } else {
      index = queue->first - 1;
   }

   // Copy into message array
   auto dst = make_p32<OSMessage>(queue->messages.value + index * sizeof(OSMessage));
   memcpy(dst, message, sizeof(OSMessage));

   // Update queue start and count
   queue->first = index;
   queue->count++;

   // Wake up anyone waiting for message to read
   lock.unlock();
   queue->waitRead->notify_one();
   return TRUE;
}

// Read from front of message queue
BOOL
OSReceiveMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags flags)
{
   std::unique_lock<std::mutex> lock { *queue->mutex };

   if (!testFlag(flags, OSMessageFlags::Blocking) && queue->count == 0) {
      return FALSE;
   }

   while (queue->count == 0) {
      queue->waitRead->wait(lock);
   }

   auto index = queue->first;
   auto src = make_p32<OSMessage>(queue->messages.value + index * sizeof(OSMessage));

   // Copy from message array
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->count--;

   // Wake up anyone waiting for space to send
   lock.unlock();
   queue->waitSend->notify_one();
   return TRUE;
}

// Peek from front of message queue
BOOL
OSPeekMessage(WMessageQueue *queue, p32<OSMessage> message)
{
   std::unique_lock<std::mutex> lock { *queue->mutex };

   if (queue->count == 0) {
      return FALSE;
   }

   // Copy from message array
   auto src = make_p32<OSMessage>(queue->messages.value + queue->first * sizeof(OSMessage));
   memcpy(message, src, sizeof(OSMessage));
   return TRUE;
}

p32<WMessageQueue>
OSGetSystemMessageQueue()
{
   return gSystemMessageQueue;
}

void
CoreInit::registerMessageQueueFunctions()
{
   RegisterSystemFunction(OSInitMessageQueue);
   RegisterSystemFunction(OSInitMessageQueueEx);
   RegisterSystemFunction(OSSendMessage);
   RegisterSystemFunction(OSJamMessage);
   RegisterSystemFunction(OSReceiveMessage);
   RegisterSystemFunction(OSPeekMessage);
   RegisterSystemFunction(OSGetSystemMessageQueue);
}

void
CoreInit::initialiseMessageQueues()
{
   gSystemMessageQueue = OSAllocFromSystem(sizeof(OSMessageQueue), 4);
   gSystemMessageArray = OSAllocFromSystem(16 * sizeof(OSMessage), 4);
   OSInitMessageQueue(gSystemMessageQueue, gSystemMessageArray, 16);
}
