#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_messagequeue.h"
#include "system.h"
#include "util.h"

static MessageQueueHandle
gSystemMessageQueue;

static OSMessage *
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

   queue->mutex = OSAllocMutex();
   queue->waitRead = OSAllocCondition();
   queue->waitSend = OSAllocCondition();

   OSInitMutex(queue->mutex);
   OSInitCond(queue->waitRead);
   OSInitCond(queue->waitSend);
}

// Insert at back of message queue
BOOL
OSSendMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   OSLockMutex(queue->mutex);

   if (!(flags & MessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      OSWaitCond(queue->waitSend, queue->mutex);
   }

   auto index = (queue->first + queue->count) % queue->size;
   auto dst = static_cast<OSMessage*>(queue->messages) + index;

   // Copy into message array
   memcpy(dst, message, sizeof(OSMessage));
   queue->count++;

   // Wake up anyone waiting for message to read
   OSUnlockMutex(queue->mutex);
   OSSignalCond(queue->waitRead);

   return TRUE;
}

// Insert at front of message queue
BOOL
OSJamMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   uint32_t index;
   OSLockMutex(queue->mutex);

   if (!(flags & MessageFlags::Blocking) && queue->count == queue->size) {
      // Do not wait for space
      OSUnlockMutex(queue->mutex);
      return FALSE;
   }

   // Wait for space
   while (queue->count == queue->size) {
      OSWaitCond(queue->waitSend, queue->mutex);
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
   OSUnlockMutex(queue->mutex);
   OSSignalCond(queue->waitRead);

   return TRUE;
}

// Read from front of message queue
BOOL
OSReceiveMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   OSLockMutex(queue->mutex);

   if (!(flags & MessageFlags::Blocking) && queue->count == 0) {
      // Do not wait for space
      OSUnlockMutex(queue->mutex);
      return FALSE;
   }

   while (queue->count == 0) {
      OSWaitCond(queue->waitRead, queue->mutex);
   }

   auto index = queue->first;
   auto src = static_cast<OSMessage*>(queue->messages) + index;

   // Copy from message array
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->count--;

   // Wake up anyone waiting for space to send
   OSUnlockMutex(queue->mutex);
   OSSignalCond(queue->waitSend);

   return TRUE;
}

// Peek from front of message queue
BOOL
OSPeekMessage(MessageQueueHandle handle, OSMessage *message)
{
   auto queue = gSystem.getSystemObject<MessageQueue>(handle);
   OSLockMutex(queue->mutex);

   if (queue->count == 0) {
      OSUnlockMutex(queue->mutex);
      return FALSE;
   }

   // Copy from message array
   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));

   OSUnlockMutex(queue->mutex);
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
   gSystemMessageQueue = OSAllocFromSystem<SystemObjectHeader>();
   gSystemMessageArray = reinterpret_cast<OSMessage*>(OSAllocFromSystem(16 * sizeof(OSMessage)));
   OSInitMessageQueue(gSystemMessageQueue, gSystemMessageArray, 16);
}
