#include "coreinit.h"
#include "coreinit_thread.h"
#include "coreinit_systeminfo.h"
#include "memory.h"
#include "processor.h"
#include "trace.h"
#include "system.h"
#include <Windows.h>

static OSThread *
gDefaultThreads[3];

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

OSThread *
OSGetCurrentThread()
{
   return gProcessor.getCurrentFiber()->thread;
}

BOOL
OSSetThreadPriority(OSThread *thread, uint32_t priority)
{
   thread->basePriority = priority;
   gProcessor.reschedule();
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
   thread->attr &= ~OSThreadAttributes::AffinityAny;
   thread->attr |= affinity;
   gProcessor.reschedule();
   return TRUE;
}

uint32_t
OSGetThreadAffinity(OSThread *thread)
{
   return thread->attr & OSThreadAttributes::AffinityAny;
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
   gProcessor.reschedule();
}

static uint32_t gThreadId = 1;

BOOL
OSCreateThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, void *argv, uint8_t *stack, uint32_t stackSize, uint32_t priority, OSThreadAttributes::Flags attributes)
{
   // Create new fiber
   auto fiber = gProcessor.createFiber();

   // Setup OSThread
   thread->entryPoint = entry;
   thread->stackEnd = gMemory.untranslate(stack);
   thread->stackStart = thread->stackEnd - stackSize;
   thread->basePriority = priority;
   thread->attr = attributes;
   thread->fiber = fiber;
   thread->state = OSThreadState::Ready;
   thread->id = gThreadId++;
   thread->suspendCounter = 1;
   fiber->thread = thread;

   // Setup ThreadState
   auto state = &fiber->state;
   memset(state, 0, sizeof(ThreadState));
   state->cia = thread->entryPoint;
   state->nia = state->cia + 4;
   state->gpr[1] = thread->stackEnd;
   state->gpr[3] = argc;
   state->gpr[4] = gMemory.untranslate(argv);

   // Initialise tracer
   traceInit(state, 128);

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
      gProcessor.queue(thread->fiber);
   } else {
      assert(0);
   }

   return counter;
}

BOOL
OSRunThread(OSThread *thread, ThreadEntryPoint entry, uint32_t argc, p32<void> argv)
{
   auto fiber = thread->fiber;
   auto state = &fiber->state;
   state->cia = thread->entryPoint;
   state->nia = state->cia + 4;
   state->gpr[1] = thread->stackEnd;
   state->gpr[3] = argc;
   state->gpr[4] = static_cast<uint32_t>(argv);
   gProcessor.queue(fiber);
   return FALSE;
}

BOOL
OSJoinThread(OSThread *thread, be_val<int> *exitValue)
{
   auto fiber = thread->fiber;
   *exitValue = gProcessor.join(fiber);
   return TRUE;
}

uint32_t
OSSuspendThread(OSThread *thread)
{
   assert(false);
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
