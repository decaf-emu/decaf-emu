#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_messagequeue.h"
#include "coreinit_scheduler.h"

const uint32_t OSMessageQueue::Tag;

static OSMessageQueue *
gSystemMessageQueue;

static OSMessage *
gSystemMessageArray;

void
OSInitMessageQueue(OSMessageQueue *queue, OSMessage *messages, int32_t size)
{
   OSInitMessageQueueEx(queue, messages, size, nullptr);
}

void
OSInitMessageQueueEx(OSMessageQueue *queue, OSMessage *messages, int32_t size, const char *name)
{
   queue->tag = OSMessageQueue::Tag;
   queue->name = name;
   queue->messages = messages;
   queue->size = size;
   queue->first = 0;
   queue->used = 0;
   OSInitThreadQueueEx(&queue->sendQueue, queue);
   OSInitThreadQueueEx(&queue->recvQueue, queue);
}

BOOL
OSSendMessage(OSMessageQueue *queue, OSMessage *message, OSMessageFlags flags)
{
   OSLockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == queue->size) {
      // Do not block waiting for space to insert message
      OSUnlockScheduler();
      return FALSE;
   }

   // Wait for space in the message queue
   while (queue->used == queue->size) {
      OSSleepThreadNoLock(&queue->sendQueue);
      OSRescheduleNoLock();
   }

   // Copy into message array
   auto index = (queue->first + queue->used) % queue->size;
   auto dst = static_cast<OSMessage*>(queue->messages) + index;
   memcpy(dst, message, sizeof(OSMessage));
   queue->used++;

   // Wakeup threads waiting to read message
   OSWakeupThreadNoLock(&queue->recvQueue);
   OSRescheduleNoLock();

   OSUnlockScheduler();
   return TRUE;
}

BOOL
OSJamMessage(OSMessageQueue *queue, OSMessage *message, OSMessageFlags flags)
{
   OSLockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == queue->size) {
      // Do not block waiting for space to insert message
      OSUnlockScheduler();
      return FALSE;
   }

   // Wait for space in the message queue
   while (queue->used == queue->size) {
      OSSleepThreadNoLock(&queue->sendQueue);
      OSRescheduleNoLock();
   }

   if (queue->first == 0) {
      queue->first = queue->size - 1;
   } else {
      queue->first--;
   }

   // Copy into message array
   auto dst = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(dst, message, sizeof(OSMessage));
   queue->used++;

   // Wakeup threads waiting to read message
   OSWakeupThreadNoLock(&queue->recvQueue);
   OSRescheduleNoLock();

   OSUnlockScheduler();
   return TRUE;
}

BOOL
OSReceiveMessage(OSMessageQueue *queue, OSMessage *message, OSMessageFlags flags)
{
   OSLockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == 0) {
      // Do not block waiting for a message to arrive
      OSUnlockScheduler();
      return FALSE;
   }

   // Wait for a message to appear in queue
   while (queue->used == 0) {
      OSSleepThreadNoLock(&queue->recvQueue);
      OSRescheduleNoLock();
   }

   // Copy into message array
   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->used--;

   // Wakeup threads waiting for space to send message
   OSWakeupThreadNoLock(&queue->sendQueue);
   OSRescheduleNoLock();

   OSUnlockScheduler();
   return TRUE;
}

BOOL
OSPeekMessage(OSMessageQueue *queue, OSMessage *message)
{
   OSLockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (queue->used == 0) {
      OSUnlockScheduler();
      return FALSE;
   }

   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));

   OSUnlockScheduler();
   return TRUE;
}

OSMessageQueue *
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
   gSystemMessageQueue = OSAllocFromSystem<OSMessageQueue>();
   gSystemMessageArray = reinterpret_cast<OSMessage*>(OSAllocFromSystem(16 * sizeof(OSMessage)));
   OSInitMessageQueue(gSystemMessageQueue, gSystemMessageArray, 16);
}
