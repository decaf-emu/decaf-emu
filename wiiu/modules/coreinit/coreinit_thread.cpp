#include "coreinit.h"
#include "coreinit_thread.h"
#include "memory.h"

p32<OSThread>
OSGetCurrentThread()
{
   return make_p32<OSThread>(nullptr);
}

void
OSInitThreadQueue(p32<OSThreadQueue> pQueue)
{
   auto queue = p32_direct(pQueue);
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = nullptr;
}

void
OSInitThreadQueueEx(p32<OSThreadQueue> pQueue, p32<void> pParent)
{
   auto queue = p32_direct(pQueue);
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = pParent;
}

static p32<void>
coreinit_memcpy(p32<void> dst, p32<const void> src, size_t size)
{
   return dst;
}

void
CoreInit::registerThreadFunctions()
{
   RegisterSystemFunction(OSGetCurrentThread);
}
