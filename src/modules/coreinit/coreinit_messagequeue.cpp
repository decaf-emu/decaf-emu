#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_messagequeue.h"
#include "coreinit_scheduler.h"

namespace coreinit
{

const uint32_t
OSMessageQueue::Tag;

static OSMessageQueue *
sSystemMessageQueue;

static OSMessage *
sSystemMessageArray;

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
   coreinit::internal::lockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == queue->size) {
      // Do not block waiting for space to insert message
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   // Wait for space in the message queue
   while (queue->used == queue->size) {
      coreinit::internal::sleepThreadNoLock(&queue->sendQueue);
      coreinit::internal::rescheduleNoLock();
   }

   // Copy into message array
   auto index = (queue->first + queue->used) % queue->size;
   auto dst = static_cast<OSMessage*>(queue->messages) + index;
   memcpy(dst, message, sizeof(OSMessage));
   queue->used++;

   // Wakeup threads waiting to read message
   coreinit::internal::wakeupThreadNoLock(&queue->recvQueue);
   coreinit::internal::rescheduleNoLock();

   coreinit::internal::unlockScheduler();
   return TRUE;
}

BOOL
OSJamMessage(OSMessageQueue *queue, OSMessage *message, OSMessageFlags flags)
{
   coreinit::internal::lockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == queue->size) {
      // Do not block waiting for space to insert message
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   // Wait for space in the message queue
   while (queue->used == queue->size) {
      coreinit::internal::sleepThreadNoLock(&queue->sendQueue);
      coreinit::internal::rescheduleNoLock();
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
   coreinit::internal::wakeupThreadNoLock(&queue->recvQueue);
   coreinit::internal::rescheduleNoLock();

   coreinit::internal::unlockScheduler();
   return TRUE;
}

BOOL
OSReceiveMessage(OSMessageQueue *queue, OSMessage *message, OSMessageFlags flags)
{
   coreinit::internal::lockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == 0) {
      // Do not block waiting for a message to arrive
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   // Wait for a message to appear in queue
   while (queue->used == 0) {
      coreinit::internal::sleepThreadNoLock(&queue->recvQueue);
      coreinit::internal::rescheduleNoLock();
   }

   // Copy into message array
   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->used--;

   // Wakeup threads waiting for space to send message
   coreinit::internal::wakeupThreadNoLock(&queue->sendQueue);
   coreinit::internal::rescheduleNoLock();

   coreinit::internal::unlockScheduler();
   return TRUE;
}

BOOL
OSPeekMessage(OSMessageQueue *queue, OSMessage *message)
{
   coreinit::internal::lockScheduler();
   assert(queue && queue->tag == OSMessageQueue::Tag);
   assert(message);

   if (queue->used == 0) {
      coreinit::internal::unlockScheduler();
      return FALSE;
   }

   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));

   coreinit::internal::unlockScheduler();
   return TRUE;
}

OSMessageQueue *
OSGetSystemMessageQueue()
{
   return sSystemMessageQueue;
}

void
Module::registerMessageQueueFunctions()
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
Module::initialiseMessageQueues()
{
   sSystemMessageQueue = coreinit::internal::sysAlloc<OSMessageQueue>();
   sSystemMessageArray = reinterpret_cast<OSMessage*>(coreinit::internal::sysAlloc(16 * sizeof(OSMessage)));
   OSInitMessageQueue(sSystemMessageQueue, sSystemMessageArray, 16);
}

} // namespace coreinit
