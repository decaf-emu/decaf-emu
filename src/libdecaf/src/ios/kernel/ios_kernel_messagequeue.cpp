#include "ios_kernel_messagequeue.h"
#include "ios_kernel_process.h"
#include "ios_kernel_scheduler.h"
#include "ios_kernel_thread.h"

#include <common/log.h>

namespace ios::kernel
{

struct StaticData
{
   be2_array<MessageQueue, MaxNumMessageQueues> queues;
   be2_val<uint32_t> numCreatedQueues;

   be2_array<MessageQueue, MaxNumThreads> perThreadQueues;
   be2_array<Message, MaxNumThreads> perThreadMesssages;
};

static phys_ptr<StaticData>
sData;


/**
 * Create a message queue.
 */
Error
IOS_CreateMessageQueue(phys_ptr<Message> messages,
                       uint32_t size)
{
   internal::lockScheduler();
   phys_ptr<MessageQueue> queue = nullptr;

   for (auto i = 0u; i < sData->queues.size(); ++i) {
      if (sData->queues[i].size == 0) {
         queue = phys_addrof(sData->queues[i]);
         queue->uid = static_cast<int32_t>((sData->numCreatedQueues << 12) | i);
         break;
      }
   }

   if (!queue) {
      internal::unlockScheduler();
      return Error::Max;
   }

   queue->first = 0u;
   queue->used = 0u;
   queue->size = size;
   queue->messages = messages;
   queue->flags = MessageQueueFlags::None;
   queue->pid = static_cast<uint8_t>(internal::getCurrentProcessId());

   ThreadQueue_Initialise(phys_addrof(queue->receiveQueue));
   ThreadQueue_Initialise(phys_addrof(queue->sendQueue));

   sData->numCreatedQueues++;
   internal::unlockScheduler();

   return static_cast<Error>(queue->uid);
}


/**
 * Destroy message queue.
 *
 * Interrupts any threads waiting on the receive or send queue.
 */
Error
IOS_DestroyMessageQueue(MessageQueueId id)
{
   internal::lockScheduler();
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueue(id, &queue);
   if (error < Error::OK) {
      internal::unlockScheduler();
      return error;
   }

   if (queue->flags & MessageQueueFlags::RegisteredEventHandler) {
      gLog->warn("Destroying queue registered to event.");

      // TODO: Unregister MessageQueue from device event handler
      queue->flags &= ~MessageQueueFlags::RegisteredEventHandler;
   }

   internal::wakeupAllThreadsNoLock(phys_addrof(queue->sendQueue),
                                    Error::Intr);

   internal::wakeupAllThreadsNoLock(phys_addrof(queue->receiveQueue),
                                    Error::Intr);

   std::memset(queue.getRawPointer(), 0, sizeof(ThreadQueue));
   internal::unlockScheduler();
   return Error::OK;
}


/**
 * Insert a message to the back of the message queue.
 */
Error
IOS_SendMessage(MessageQueueId id,
                Message message,
                MessageFlags flags)
{
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueue(id, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::sendMessage(queue, message, flags);
}


/**
 * Insert a message to front of the message queue.
 */
Error
IOS_JamMessage(MessageQueueId id,
               Message message,
               MessageFlags flags)
{
   internal::lockScheduler();
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueue(id, &queue);
   if (error < Error::OK) {
      internal::unlockScheduler();
      return error;
   }

   while (queue->used == queue->size) {
      if (flags & MessageFlags::NonBlocking) {
         return Error::Max;
      }

      internal::sleepThreadNoLock(phys_addrof(queue->sendQueue));
      internal::rescheduleSelfNoLock();

      auto thread = internal::getCurrentThread();
      if (thread->context.queueWaitResult != Error::OK) {
         internal::unlockScheduler();
         return thread->context.queueWaitResult;
      }
   }

   if (queue->first == 0) {
      queue->first = queue->size - 1;
   } else {
      queue->first--;
   }

   queue->messages[queue->first] = message;
   queue->used++;

   internal::wakeupOneThreadNoLock(phys_addrof(queue->receiveQueue),
                                   Error::OK);
   internal::rescheduleAllNoLock();
   internal::unlockScheduler();
   return Error::OK;
}


/**
 * Receive a message from the front of the message queue.
 */
Error
IOS_ReceiveMessage(MessageQueueId id,
                   phys_ptr<Message> message,
                   MessageFlags flags)
{
   phys_ptr<MessageQueue> queue;
   auto error = internal::getMessageQueue(id, &queue);
   if (error < Error::OK) {
      return error;
   }

   return internal::receiveMessage(queue, message, flags);
}


namespace internal
{

/**
 * Find a message queue from it's ID.
 */
Error
getMessageQueue(MessageQueueId id,
                phys_ptr<MessageQueue> *outQueue)
{
   id &= 0xFFF;

   if (id >= sData->queues.size()) {
      return Error::Invalid;
   }

   auto queue = phys_addrof(sData->queues[id]);
   if (queue->pid != internal::getCurrentProcessId()) {
      // Can only access queues belonging to same process.
      return Error::Access;
   }

   if (outQueue) {
      *outQueue = queue;
   }

   return Error::OK;
}


/**
 * Get the message queue for this thread.
 *
 * Used for blocking requests.
 */
phys_ptr<MessageQueue>
getCurrentThreadMessageQueue()
{
   return phys_addrof(sData->perThreadQueues[internal::getCurrentThreadId()]);
}


/**
 * Insert message to the back of the message queue.
 */
Error
sendMessage(phys_ptr<MessageQueue> queue,
            Message message,
            MessageFlags flags)
{
   internal::lockScheduler();
   while (queue->used == queue->size) {
      if (flags & MessageFlags::NonBlocking) {
         internal::unlockScheduler();
         return Error::Max;
      }

      internal::sleepThreadNoLock(phys_addrof(queue->sendQueue));
      internal::rescheduleSelfNoLock();

      auto thread = internal::getCurrentThread();
      if (thread->context.queueWaitResult != Error::OK) {
         internal::unlockScheduler();
         return thread->context.queueWaitResult;
      }
   }

   auto index = (queue->first + queue->used) % queue->size;
   queue->messages[index] = message;
   queue->used++;

   internal::wakeupOneThreadNoLock(phys_addrof(queue->receiveQueue),
                                   Error::OK);
   internal::rescheduleAllNoLock();
   internal::unlockScheduler();
   return Error::OK;
}


/**
 * Receive a message from the front of the message queue.
 */
Error
receiveMessage(phys_ptr<MessageQueue> queue,
               phys_ptr<Message> message,
               MessageFlags flags)
{
   internal::lockScheduler();
   while (queue->used == 0) {
      if (flags & MessageFlags::NonBlocking) {
         internal::unlockScheduler();
         return Error::Max;
      }

      internal::sleepThreadNoLock(phys_addrof(queue->receiveQueue));
      internal::rescheduleSelfNoLock();

      auto thread = internal::getCurrentThread();
      if (thread->context.queueWaitResult != Error::OK) {
         internal::unlockScheduler();
         return thread->context.queueWaitResult;
      }
   }

   *message = queue->messages[queue->first];
   queue->first = (queue->first + 1) % queue->size;
   queue->used--;

   internal::wakeupOneThreadNoLock(phys_addrof(queue->sendQueue),
                                   Error::OK);
   internal::rescheduleAllNoLock();
   internal::unlockScheduler();
   return Error::OK;
}

void
initialiseStaticMessageQueueData()
{
   sData = allocProcessStatic<StaticData>();

   for (auto i = 0u; i < sData->perThreadQueues.size(); ++i) {
      auto &queue = sData->perThreadQueues[i];

      queue.used = 0u;
      queue.first = 0u;
      queue.size = 1u;
      queue.messages = phys_addrof(sData->perThreadMesssages[i]);
      queue.uid = -4;
      queue.pid = uint8_t { 0 };
      queue.flags = MessageQueueFlags::None;
      queue.unk0x1E = uint16_t { 0 };

      ThreadQueue_Initialise(phys_addrof(queue.receiveQueue));
      ThreadQueue_Initialise(phys_addrof(queue.sendQueue));
   }
}

} // namespace internal

} // namespace ios::kernel
