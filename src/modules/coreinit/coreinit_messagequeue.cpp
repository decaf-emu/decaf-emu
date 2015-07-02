#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_messagequeue.h"
#include "system.h"
#include "util.h"

MessageQueueHandle
gSystemMessageQueue;

OSMessage *
gSystemMessageArray;

void
OSInitMessageQueue(MessageQueueHandle handle, OSMessage *messages, int32_t size)
{
   OSInitMessageQueueEx(handle, messages, size, nullptr);
}

void
OSInitMessageQueueEx(MessageQueueHandle handle, OSMessage *messages, int32_t size, char *name)
{
   auto queue = gSystem.addSystemObject<MessageQueue>(handle);
   queue->name = name;
   queue->first = 0;
   queue->count = 0;
   queue->size = size;
   queue->messages = messages;
}

// Insert at back of message queue
BOOL
OSSendMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   std::unique_lock<std::mutex> lock { queue->mutex };

   if (!(flags & MessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      queue->waitSend.wait(lock);
   }

   auto index = (queue->first + queue->count) % queue->size;
   auto dst = static_cast<OSMessage*>(queue->messages) + index;

   // Copy into message array
   memcpy(dst, message, sizeof(OSMessage));
   queue->count++;

   // Wake up anyone waiting for message to read
   lock.unlock();
   queue->waitRead.notify_one();
   return TRUE;
}

// Insert at front of message queue
BOOL
OSJamMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   std::unique_lock<std::mutex> lock { queue->mutex };
   uint32_t index;

   if (!(flags & MessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      queue->waitSend.wait(lock);
   }

   // Get index before front
   if (queue->first == 0) {
      index = queue->size - 1;
   } else {
      index = queue->first - 1;
   }

   // Copy into message array
   auto dst = static_cast<OSMessage*>(queue->messages) + index;
   memcpy(dst, message, sizeof(OSMessage));

   // Update queue start and count
   queue->first = index;
   queue->count++;

   // Wake up anyone waiting for message to read
   lock.unlock();
   queue->waitRead.notify_one();
   return TRUE;
}

// Read from front of message queue
BOOL
OSReceiveMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   std::unique_lock<std::mutex> lock { queue->mutex };

   if (!(flags & MessageFlags::Blocking) && queue->count == 0) {
      return FALSE;
   }

   while (queue->count == 0) {
      queue->waitRead.wait(lock);
   }

   auto index = queue->first;
   auto src = static_cast<OSMessage*>(queue->messages) + index;

   // Copy from message array
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->count--;

   // Wake up anyone waiting for space to send
   lock.unlock();
   queue->waitSend.notify_one();
   return TRUE;
}

// Peek from front of message queue
BOOL
OSPeekMessage(MessageQueueHandle handle, OSMessage *message)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   std::unique_lock<std::mutex> lock { queue->mutex };

   if (queue->count == 0) {
      return FALSE;
   }

   // Copy from message array
   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));
   return TRUE;
}

MessageQueueHandle
OSGetSystemMessageQueue()
{
   return gSystemMessageQueue;
}

void
CoreInit::registerMessageQueueFunctions()
{
   RegisterKernelFunction(OSInitMessageQueue);
   RegisterKernelFunction(OSInitMessageQueueEx);
   RegisterKernelFunction(OSSendMessage);
   RegisterKernelFunction(OSJamMessage);
   RegisterKernelFunction(OSReceiveMessage);
   RegisterKernelFunction(OSPeekMessage);
   RegisterKernelFunction(OSGetSystemMessageQueue);
}

void
CoreInit::initialiseMessageQueues()
{
   gSystemMessageQueue = make_p32<void>(OSAllocFromSystem<SystemObjectHeader>());
   gSystemMessageArray = reinterpret_cast<OSMessage*>(OSAllocFromSystem(16 * sizeof(OSMessage)));
   OSInitMessageQueue(gSystemMessageQueue, gSystemMessageArray, 16);
}
