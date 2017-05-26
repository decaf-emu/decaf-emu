#include "coreinit.h"
#include "coreinit_memheap.h"
#include "coreinit_messagequeue.h"
#include "coreinit_scheduler.h"
#include <common/decaf_assert.h>
#include <array>

namespace coreinit
{

const uint32_t
OSMessageQueue::Tag;

static OSMessageQueue *
sSystemMessageQueue;

static std::array<OSMessage, 16> *
sSystemMessageArray;


/**
 * Initialise a message queue structure.
 */
void
OSInitMessageQueue(OSMessageQueue *queue,
                   OSMessage *messages,
                   int32_t size)
{
   OSInitMessageQueueEx(queue, messages, size, nullptr);
}


/**
 * Initialise a message queue structure with a name.
 */
void
OSInitMessageQueueEx(OSMessageQueue *queue,
                     OSMessage *messages,
                     int32_t size,
                     const char *name)
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


/**
 * Insert a message into the queue.
 *
 * If the OSMessageFlags::HighPriority flag is set then the current thread will
 * block until there is space in the queue to insert the message, else it will
 * return immediately with the return value of FALSE.
 *
 * If the OSMessageFlags::HighPriority flag is set then the message will be
 * inserted at the front of the queue, otherwise it will be inserted at the back.
 *
 * \return Returns TRUE if the message was inserted in the queue.
 */
BOOL
OSSendMessage(OSMessageQueue *queue,
              OSMessage *message,
              OSMessageFlags flags)
{
   unsigned index;
   internal::lockScheduler();
   decaf_check(queue && queue->tag == OSMessageQueue::Tag);
   decaf_check(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == queue->size) {
      // Do not block waiting for space to insert message.
      internal::unlockScheduler();
      return FALSE;
   }

   // Wait for space in the message queue.
   while (queue->used == queue->size) {
      internal::sleepThreadNoLock(&queue->sendQueue);
      internal::rescheduleSelfNoLock();
   }

   if (flags & OSMessageFlags::HighPriority) {
      // High priorty messages are pushed to the front of the queue.
      if (queue->first == 0) {
         queue->first = queue->size - 1;
      } else {
         queue->first--;
      }

      index = queue->first;
   } else {
      // Normal messages are pushed to back of the queue.
      index = (queue->first + queue->used) % queue->size;
   }

   std::memcpy(queue->messages + index, message, sizeof(OSMessage));
   queue->used++;

   // Wakeup threads waiting to read message
   internal::wakeupThreadNoLock(&queue->recvQueue);
   internal::rescheduleAllCoreNoLock();

   internal::unlockScheduler();
   return TRUE;
}


/**
 * Read and remove a message from the queue.
 *
 * If flags has OSMessageFlags::Blocking then the current thread will block
 * until there is a mesasge in the queue to read, else it will return
 * immediately with the return value of FALSE.
 *
 * \return Returns TRUE if a message was read from the queue.
 */
BOOL
OSReceiveMessage(OSMessageQueue *queue,
                 OSMessage *message,
                 OSMessageFlags flags)
{
   internal::lockScheduler();
   decaf_check(queue && queue->tag == OSMessageQueue::Tag);
   decaf_check(message);

   if (!(flags & OSMessageFlags::Blocking) && queue->used == 0) {
      // Do not block waiting for a message to arrive
      internal::unlockScheduler();
      return FALSE;
   }

   // Wait for a message to appear in queue
   while (queue->used == 0) {
      internal::sleepThreadNoLock(&queue->recvQueue);
      internal::rescheduleSelfNoLock();
   }

   decaf_check(queue->used > 0);

   // Copy into message array
   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));
   queue->first = (queue->first + 1) % queue->size;
   queue->used--;

   // Wakeup threads waiting for space to send message
   internal::wakeupThreadNoLock(&queue->sendQueue);
   internal::rescheduleAllCoreNoLock();

   internal::unlockScheduler();
   return TRUE;
}


/**
 * Read and do NOT remove a message from the queue.
 *
 * \return Returns TRUE if a message was read from the queue.
 */
BOOL
OSPeekMessage(OSMessageQueue *queue, OSMessage *message)
{
   internal::lockScheduler();
   decaf_check(queue && queue->tag == OSMessageQueue::Tag);
   decaf_check(message);

   if (queue->used == 0) {
      internal::unlockScheduler();
      return FALSE;
   }

   auto src = static_cast<OSMessage*>(queue->messages) + queue->first;
   memcpy(message, src, sizeof(OSMessage));

   internal::unlockScheduler();
   return TRUE;
}


/**
 * Get the global system message queue.
 */
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
   RegisterKernelFunction(OSReceiveMessage);
   RegisterKernelFunction(OSPeekMessage);
   RegisterKernelFunction(OSGetSystemMessageQueue);

   RegisterInternalData(sSystemMessageQueue);
   RegisterInternalData(sSystemMessageArray);
}

void
Module::initialiseMessageQueues()
{
   OSInitMessageQueue(sSystemMessageQueue, sSystemMessageArray->data(), 16);
}

} // namespace coreinit
