#include "thread.h"
#include "interpreter.h"
#include "system.h"
#include "modules/coreinit/coreinit_memory.h"

__declspec(thread) Thread *tCurrentThread = nullptr;

Thread::Thread(System *system, uint32_t stackSize, uint32_t entryPoint) :
   mSystem(system)
{
   // Setup stack
   mStackSize = stackSize;
   mStackBase = static_cast<uint32_t>(OSAllocFromSystem(stackSize, 8));

   // Setup thread state
   mState = reinterpret_cast<ThreadState*>(&mAlignedState);
   memset(mState, 0, sizeof(ThreadState));
   mState->thread = this;
   mState->cia = entryPoint;
   mState->nia = entryPoint + 4;

   // Set r1 to stack end
   mState->gpr[1] = mStackBase + mStackSize;

   // Allocate OSThread
   mOSThread = OSAllocFromSystem(sizeof(OSThread), 8);
   memset(mOSThread, 0, sizeof(OSThread));

   // Do we actually have to make valid data inside OSThread?
   mOSThread->entryPoint = entryPoint;
   mOSThread->stackStart = mStackBase;
   mOSThread->stackEnd = mStackBase + mStackSize;
}

Thread::~Thread()
{
   OSFreeToSystem(make_p32<void>(mStackBase));
}

Thread *
Thread::getCurrentThread()
{
   return tCurrentThread;
}

System *
Thread::getSystem() const
{
   return mSystem;
}

ThreadState *
Thread::getThreadState() const
{
   return mState;
}

p32<OSThread>
Thread::getOSThread() const
{
   return mOSThread;
}

void
Thread::run(Interpreter &interpreter)
{
   tCurrentThread = this;
   interpreter.execute(mState, mState->cia);
}
