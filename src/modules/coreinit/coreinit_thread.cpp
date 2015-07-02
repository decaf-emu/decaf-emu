#include "coreinit.h"
#include "coreinit_thread.h"
#include "coreinit_systeminfo.h"
#include "memory.h"
#include "thread.h"
#include "system.h"
#include <Windows.h>

static OSThread *
gDefaultThreads[3];

OSThread *
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
OSSetThreadAffinity(OSThread *thread, uint32_t affinity)
{
   thread->attr &= ~OSThreadAttributes::AffinityNone;
   thread->attr |= affinity;
   return TRUE;
}

uint32_t
OSGetThreadAffinity(OSThread *thread)
{
   return thread->attr & OSThreadAttributes::AffinityNone;
}

void
OSSleepTicks(Time ticks)
{
   // TODO: Figure out ticks to time translation
   std::this_thread::sleep_for(std::chrono::nanoseconds(100));
}

void
OSYieldThread()
{
   std::this_thread::yield();
}

BOOL
OSCreateThread(OSThread *osThread, ThreadEntryPoint entry, uint32_t argc, void *argv, uint8_t *stack, uint32_t stackSize, uint32_t priority, OSThreadAttributes::Flags attributes)
{
   auto thread = new Thread(osThread, entry, argc, argv, stack, stackSize, priority, attributes);
   gSystem.addThread(thread);
   return TRUE;
}

void
OSSetDefaultThread(uint32_t coreID, OSThread *thread)
{
   gDefaultThreads[coreID] = thread;
}

OSThread *
OSGetDefaultThread(uint32_t coreID)
{
   if (coreID >= 3) {
      return nullptr;
   }

   return gDefaultThreads[coreID];
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
   auto counter = thread->suspendCounter;

   thread->suspendCounter--;

   if (thread->suspendCounter < 0) {
      thread->suspendCounter = 0;
   }

   if (thread->suspendCounter == 0) {
      thread->thread->resume();
   } else {
      assert(0);
   }

   return counter;
}

BOOL
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, p32<void> argv)
{
   return thread->thread->run(entry, argc, argv);
}

BOOL
OSJoinThread(OSThread *thread, be_val<int> *exitValue)
{
   *exitValue = thread->thread->join();
   return TRUE;
}

uint32_t
OSSuspendThread(OSThread *thread)
{
   auto old = thread->suspendCounter;
   thread->suspendCounter++;

   if (thread->suspendCounter > 0) {
      // TODO: Suspend!
      return -1;
   }

   return old;
}

void
CoreInit::registerThreadFunctions()
{
   RegisterKernelFunction(OSGetCurrentThread);
   RegisterKernelFunction(OSInitThreadQueue);
   RegisterKernelFunction(OSInitThreadQueueEx);
   RegisterKernelFunction(OSGetDefaultThread);
   RegisterKernelFunction(OSGetThreadSpecific);
   RegisterKernelFunction(OSSetThreadSpecific);
   RegisterKernelFunction(OSGetThreadPriority);
   RegisterKernelFunction(OSSetThreadPriority);
   RegisterKernelFunction(OSGetThreadAffinity);
   RegisterKernelFunction(OSSetThreadAffinity);
   RegisterKernelFunction(OSGetThreadName);
   RegisterKernelFunction(OSSetThreadName);
   RegisterKernelFunction(OSCreateThread);
   RegisterKernelFunction(OSSleepTicks);
   RegisterKernelFunction(OSResumeThread);
   RegisterKernelFunction(OSRunThread);
   RegisterKernelFunction(OSJoinThread);
   RegisterKernelFunction(OSYieldThread);
}
