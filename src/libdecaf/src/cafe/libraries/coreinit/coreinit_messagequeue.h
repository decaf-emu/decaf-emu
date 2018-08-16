#pragma once
#include "coreinit_enum.h"
#include "coreinit_thread.h"
#include "coreinit_mutex.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_messagequeue Message Queue
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSMessage
{
   be2_virt_ptr<void> message;
   be2_val<uint32_t> args[3];
};
CHECK_OFFSET(OSMessage, 0x00, message);
CHECK_OFFSET(OSMessage, 0x04, args);
CHECK_SIZE(OSMessage, 0x10);

struct OSMessageQueue
{
   static constexpr auto Tag = 0x6D536751u;

   be2_val<uint32_t> tag;
   be2_virt_ptr<const char> name;
   UNKNOWN(4);
   be2_struct<OSThreadQueue> sendQueue;
   be2_struct<OSThreadQueue> recvQueue;
   be2_virt_ptr<OSMessage> messages;
   be2_val<uint32_t> size;
   be2_val<uint32_t> first;
   be2_val<uint32_t> used;
};
CHECK_OFFSET(OSMessageQueue, 0x00, tag);
CHECK_OFFSET(OSMessageQueue, 0x04, name);
CHECK_OFFSET(OSMessageQueue, 0x0c, sendQueue);
CHECK_OFFSET(OSMessageQueue, 0x1c, recvQueue);
CHECK_OFFSET(OSMessageQueue, 0x2c, messages);
CHECK_OFFSET(OSMessageQueue, 0x30, size);
CHECK_OFFSET(OSMessageQueue, 0x34, first);
CHECK_OFFSET(OSMessageQueue, 0x38, used);
CHECK_SIZE(OSMessageQueue, 0x3c);

#pragma pack(pop)

void
OSInitMessageQueue(virt_ptr<OSMessageQueue> queue,
                   virt_ptr<OSMessage> messages,
                   uint32_t size);

void
OSInitMessageQueueEx(virt_ptr<OSMessageQueue> queue,
                     virt_ptr<OSMessage> messages,
                     uint32_t size,
                     virt_ptr<const char> name);

BOOL
OSSendMessage(virt_ptr<OSMessageQueue> queue,
              virt_ptr<OSMessage> message,
              OSMessageFlags flags);

BOOL
OSReceiveMessage(virt_ptr<OSMessageQueue> queue,
                 virt_ptr<OSMessage> message,
                 OSMessageFlags flags);

BOOL
OSPeekMessage(virt_ptr<OSMessageQueue> queue,
              virt_ptr<OSMessage> message);

/** @} */

} // namespace cafe::coreinit
