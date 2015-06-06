#include "systemthread.h"
#include "interpreter.h"
#include "system.h"
#include "modules/coreinit/coreinit_memory.h"

__declspec(thread) SystemThread *tCurrentThread = nullptr;

SystemThread *SystemThread::getCurrentThread()
{
   return tCurrentThread;
}

SystemThread::SystemThread(Module *module, uint32_t stackSize, uint32_t entryPoint)
{
   gSystem.addThread(this);

   // Setup stack
   mStackSize = stackSize;
   mStackBase = OSAllocFromSystem(stackSize, 8).value;

   // Setup thread state
   memset(&mState, 0, sizeof(ThreadState));
   mState.module = module;
   mState.cia = entryPoint;
   mState.nia = entryPoint + 4;

   // Set r1 to stack end
   mState.gpr[1] = mStackBase + mStackSize;

   // Allocate OSThread
   mOSThread = OSAllocFromSystem(sizeof(OSThread), 8);
   memset(mOSThread, 0, sizeof(OSThread));

   // Do we actually have to make valid data inside OSThread?
   mOSThread->entryPoint = entryPoint;
   mOSThread->stackStart = mStackBase;
   mOSThread->stackEnd = mStackBase + mStackSize;
}

SystemThread::~SystemThread()
{
   gSystem.removeThread(this);
}

ThreadState *
SystemThread::getThreadState()
{
   return &mState;
}

p32<OSThread>
SystemThread::getOSThread()
{
   return mOSThread;
}

void
SystemThread::run(Interpreter &interpreter)
{
   tCurrentThread = this;
   interpreter.execute(&mState, mState.cia);
}
