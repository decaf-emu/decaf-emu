#include "coreinit.h"
#include "coreinit_thread.h"
#include "memory.h"
#include "thread.h"

p32<OSThread>
OSGetCurrentThread()
{
   return Thread::getCurrentThread()->getOSThread();
}

void
OSInitThreadQueue(OSThreadQueue *queue)
{
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = nullptr;
}

void
OSInitThreadQueueEx(OSThreadQueue *queue, p32<void> parent)
{
   queue->head = nullptr;
   queue->tail = nullptr;
   queue->parent = parent;
}

BOOL
OSSetThreadPriority(OSThread *thread, uint32_t priority)
{
   thread->basePriority = priority;
   return TRUE;
}

uint32_t
OSGetThreadPriority(OSThread *thread)
{
   return thread->basePriority;
}

uint32_t
OSGetThreadSpecific(uint32_t id)
{
   return OSGetCurrentThread()->specific[id];
}

void
OSSetThreadSpecific(uint32_t id, uint32_t value)
{
   OSGetCurrentThread()->specific[id] = value;
}

void
CoreInit::registerThreadFunctions()
{
   RegisterSystemFunction(OSGetCurrentThread);
   RegisterSystemFunction(OSInitThreadQueue);
   RegisterSystemFunction(OSInitThreadQueueEx);
   RegisterSystemFunction(OSGetThreadSpecific);
   RegisterSystemFunction(OSSetThreadSpecific);
   RegisterSystemFunction(OSGetThreadPriority);
   RegisterSystemFunction(OSSetThreadPriority);
}
