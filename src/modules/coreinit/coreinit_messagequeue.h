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

#pragma pack(pop)

namespace MessageFlags
{
enum Flags
{
   Blocking     = 1 << 0,
   HighPriority = 1 << 1,
};
}

struct MessageQueue : public SystemObject
{
   static const uint32_t Tag = 0x6D536751;

   char *name;
   uint32_t first;
   uint32_t count;
   uint32_t size;
   OSMessage *messages;
   std::mutex mutex;
   std::condition_variable waitSend;
   std::condition_variable waitRead;
};

using MessageQueueHandle = p32<SystemObjectHeader>;

void
OSInitMessageQueue(MessageQueueHandle handle, OSMessage *messages, int32_t size);

void
OSInitMessageQueueEx(MessageQueueHandle handle, OSMessage *messages, int32_t size, char *name);

BOOL
OSSendMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags);

BOOL
OSJamMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags);

BOOL
OSReceiveMessage(MessageQueueHandle handle, OSMessage *message, MessageFlags::Flags flags);

BOOL
OSPeekMessage(MessageQueueHandle handle, OSMessage *message);

MessageQueueHandle
OSGetSystemMessageQueue();
