#pragma once
#include "ios_kernel_enum.h"
#include "ios_kernel_threadqueue.h"
#include "ios/ios_enum.h"

#include <common/cbool.h>
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

static constexpr auto MaxNumMessageQueues = 750u;

struct Thread;

using MessageQueueId = int32_t;
using Message = uint32_t;

struct MessageQueue
{
   be2_struct<ThreadQueue> receiveQueue;
   be2_struct<ThreadQueue> sendQueue;
   be2_val<uint32_t> used;
   be2_val<uint32_t> first;
   be2_val<uint32_t> size;
   be2_phys_ptr<Message> messages;
   be2_val<int32_t> uid;
   be2_val<uint8_t> pid;
   be2_val<MessageQueueFlags> flags;
   be2_val<uint16_t> unk0x1E;
};
CHECK_OFFSET(MessageQueue, 0x00, receiveQueue);
CHECK_OFFSET(MessageQueue, 0x04, sendQueue);
CHECK_OFFSET(MessageQueue, 0x08, used);
CHECK_OFFSET(MessageQueue, 0x0C, first);
CHECK_OFFSET(MessageQueue, 0x10, size);
CHECK_OFFSET(MessageQueue, 0x14, messages);
CHECK_OFFSET(MessageQueue, 0x18, uid);
CHECK_OFFSET(MessageQueue, 0x1C, pid);
CHECK_OFFSET(MessageQueue, 0x1D, flags);
CHECK_OFFSET(MessageQueue, 0x1E, unk0x1E);
CHECK_SIZE(MessageQueue, 0x20);

Error
IOS_CreateMessageQueue(phys_ptr<Message> messageBuffer,
                       uint32_t numMessages);

Error
IOS_DestroyMessageQueue(MessageQueueId id);

Error
IOS_SendMessage(MessageQueueId id,
                Message message,
                MessageFlags flags);

Error
IOS_JamMessage(MessageQueueId id,
               Message message,
               MessageFlags flags);

Error
IOS_ReceiveMessage(MessageQueueId id,
                   phys_ptr<Message> message,
                   MessageFlags flags);

namespace internal
{

phys_ptr<MessageQueue>
getMessageQueue(MessageQueueId id);

phys_ptr<MessageQueue>
getCurrentThreadMessageQueue();

Error
sendMessage(phys_ptr<MessageQueue> queue,
            Message message,
            MessageFlags flags);

Error
receiveMessage(phys_ptr<MessageQueue> queue,
               phys_ptr<Message> message,
               MessageFlags flags);

void
kernelInitialiseMessageQueue();

} // namespace internal

} // namespace ios::kernel
