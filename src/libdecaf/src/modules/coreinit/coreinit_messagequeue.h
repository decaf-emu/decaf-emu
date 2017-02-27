#pragma once
#include "coreinit_enum.h"
#include "coreinit_thread.h"
#include "coreinit_mutex.h"

#include <common/be_ptr.h>
#include <common/be_val.h>
#include <common/structsize.h>
#include <cstdint>

namespace coreinit
{

/**
 * \defgroup coreinit_messagequeue Message Queue
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSMessage
{
   be_ptr<void> message;
   be_val<uint32_t> args[3];
};
CHECK_OFFSET(OSMessage, 0x00, message);
CHECK_OFFSET(OSMessage, 0x04, args);
CHECK_SIZE(OSMessage, 0x10);

struct OSMessageQueue
{
   static const uint32_t Tag = 0x6D536751;

   be_val<uint32_t> tag;
   be_ptr<const char> name;
   UNKNOWN(4);
   OSThreadQueue sendQueue;
   OSThreadQueue recvQueue;
   be_ptr<OSMessage> messages;
   be_val<uint32_t> size;
   be_val<uint32_t> first;
   be_val<uint32_t> used;
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
OSInitMessageQueue(OSMessageQueue *queue,
                   OSMessage *messages,
                   int32_t size);

void
OSInitMessageQueueEx(OSMessageQueue *queue,
                     OSMessage *messages,
                     int32_t size,
                     const char *name);

BOOL
OSSendMessage(OSMessageQueue *queue,
              OSMessage *message,
              OSMessageFlags flags);

BOOL
OSJamMessage(OSMessageQueue *queue,
             OSMessage *message,
             OSMessageFlags flags);

BOOL
OSReceiveMessage(OSMessageQueue *queue,
                 OSMessage *message,
                 OSMessageFlags flags);

BOOL
OSPeekMessage(OSMessageQueue *queue,
              OSMessage *message);

OSMessageQueue *
OSGetSystemMessageQueue();

/** @} */

} // namespace coreinit
