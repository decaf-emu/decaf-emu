#pragma once
#include <condition_variable>
#include <mutex>
#include "systemtypes.h"
#include "coreinit_thread.h"
#include "util.h"

#pragma pack(push, 1)

struct OSMessage
{
   p32<void> message;
   uint32_t args[3];
};

// OSInitMessageQueue
// OSInitMessageQueueEx
struct OSMessageQueue
{
   static const uint32_t Tag = 0x6D536751;

   uint32_t tag;
   p32<char> name;
   UNKNOWN(4);
   OSThreadQueue threadQueue1;
   OSThreadQueue threadQueue2; // Woken up by OSSendMessage
   p32<OSMessage> messages;
   int32_t size;  // Size of messages array
   int32_t head; // First message in queue
   int32_t consumed; // Number of messages consumed
};
CHECK_OFFSET(OSMessageQueue, 0x00, tag);
CHECK_OFFSET(OSMessageQueue, 0x04, name);
CHECK_OFFSET(OSMessageQueue, 0x0C, threadQueue1);
CHECK_OFFSET(OSMessageQueue, 0x1C, threadQueue2);
CHECK_OFFSET(OSMessageQueue, 0x2C, messages);
CHECK_OFFSET(OSMessageQueue, 0x30, size);
CHECK_OFFSET(OSMessageQueue, 0x34, head);
CHECK_OFFSET(OSMessageQueue, 0x38, consumed);
CHECK_SIZE(OSMessageQueue, 0x3C);

struct WMessageQueue
{
   static const uint32_t Tag = 0x6D536751;

   uint32_t tag;
   p32<char> name;
   uint32_t first;
   uint32_t count;
   uint32_t size;
   p32<OSMessage> messages;
   std::unique_ptr<std::mutex> mutex;
   std::unique_ptr<std::condition_variable> waitSend;
   std::unique_ptr<std::condition_variable> waitRead;
};
static_assert(sizeof(WMessageQueue) <= sizeof(OSMessageQueue), "WMessageQueue must fit in OSMessageQueue");

#pragma pack(pop)

namespace OSMessageFlags
{
enum Flags
{
   Blocking     = 1 << 0,
   HighPriority = 1 << 1,
};
}

void
OSInitMessageQueue(WMessageQueue *queue, p32<OSMessage> messages, int32_t size);

void
OSInitMessageQueueEx(WMessageQueue *queue, p32<OSMessage> messages, int32_t size, char *name);

BOOL
OSSendMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags::Flags flags);

BOOL
OSJamMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags::Flags flags);

BOOL
OSReceiveMessage(WMessageQueue *queue, p32<OSMessage> message, OSMessageFlags::Flags flags);

BOOL
OSPeekMessage(WMessageQueue *queue, p32<OSMessage> message);

p32<WMessageQueue>
OSGetSystemMessageQueue();
