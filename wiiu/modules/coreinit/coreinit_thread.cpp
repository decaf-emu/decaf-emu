#include "coreinit.h"
#include "coreinit_thread.h"
#include "memory.h"
#include "thread.h"
#include <Windows.h>

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

BOOL
OSSetThreadAffinity(OSThread *thread, Flags<OS_THREAD_ATTR> affinity)
{
   Flags<OS_THREAD_ATTR> cur = thread->attr.value();
   thread->attr = affinity | (cur & ~OS_THREAD_ATTR::AFFINITY_NONE);
   return TRUE;
}

OS_THREAD_ATTR
OSGetThreadAffinity(OSThread *thread)
{
   return thread->attr;
}

void
OSSleepTicks(TimeTicks ticks)
{
}

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, void *stack, uint32_t stackSize, uint32_t priority, OS_THREAD_ATTR attributes)
{
   return FALSE;
}

void
OSSetThreadName(OSThread *thread, const char *name)
{
   thread->name = name;
}

const char *
OSGetThreadName(OSThread *thread)
{
   return thread->name;
}

uint32_t
OSResumeThread(OSThread *thread)
{
   return 0;
}

uint32_t
OSSuspendThread(OSThread *thread)
{
   return 0;
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
   RegisterSystemFunction(OSGetThreadAffinity);
   RegisterSystemFunction(OSSetThreadAffinity);
   RegisterSystemFunction(OSGetThreadName);
   RegisterSystemFunction(OSSetThreadName);
   RegisterSystemFunction(OSCreateThread);
   RegisterSystemFunction(OSSleepTicks);
}
