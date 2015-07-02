#include <atomic>
#include "thread.h"
#include "interpreter.h"
#include "system.h"
#include "modules/coreinit/coreinit_memheap.h"

__declspec(thread) Thread *tCurrentThread = nullptr;
static std::atomic<short> gThreadId = 1;

Thread::Thread(OSThread *thread, ThreadEntryPoint entry,
               uint32_t argc, void *argv,
               uint8_t *stack, uint32_t stackSize,
               uint32_t priority, OSThreadAttributes::Flags attributes) :
   mOSThread(thread)
{
   // Setup stack
   mStackSize = stackSize;
   mStackBase = gMemory.untranslate(stack) - stackSize;

   // Setup thread state
   mState = reinterpret_cast<ThreadState*>(&mAlignedState);
   memset(mState, 0, sizeof(ThreadState));
   mState->thread = this;
   mState->cia = entry;
   mState->nia = mState->cia + 4;

   // Setup registers
   mState->gpr[1] = mStackBase + mStackSize;
   mState->gpr[3] = argc;
   mState->gpr[4] = gMemory.untranslate(argv);

   // Fill out data of OSThread
   mOSThread->entryPoint = entry;
   mOSThread->stackStart = mStackBase;
   mOSThread->stackEnd = mStackBase + mStackSize;
   mOSThread->basePriority = priority;
   mOSThread->attr = attributes;
   mOSThread->thread = this;
   mOSThread->state = OSThreadState::Ready;
   mOSThread->id = gThreadId++;
   mOSThread->suspendCounter = 1;

   // Set CoreID
   if (attributes & OSThreadAttributes::AffinityCPU0) {
      mCoreID = 0;
   }

   if (attributes & OSThreadAttributes::AffinityCPU2) {
      mCoreID = 2;
   }

   if (attributes & OSThreadAttributes::AffinityCPU1) {
      mCoreID = 1;
   }
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

uint32_t Thread::getCoreID() const
{
   return mCoreID;
}

ThreadState *
Thread::getThreadState() const
{
   return mState;
}

OSThread *
Thread::getOSThread() const
{
   return mOSThread;
}

bool
Thread::run(ThreadEntryPoint entry, uint32_t argc, p32<void> argv)
{
   if (mOSThread->state != OSThreadState::Ready) {
      return false;
   }

   mState->cia = entry;
   mState->nia = mState->cia + 4;
   mState->gpr[3] = argc;
   mState->gpr[4] = static_cast<uint32_t>(argv);
   mOSThread->suspendCounter = 0;
   resume();
   return true;
}

void
Thread::resume()
{
   assert(mOSThread->suspendCounter == 0);
   assert(mOSThread->state == OSThreadState::Ready);
   mOSThread->state = OSThreadState::Running;
   mThread = std::thread(std::bind(&Thread::entry, this));
}

uint32_t
Thread::join()
{
   mThread.join();
   return mOSThread->exitValue;
}

void
Thread::entry()
{
   tCurrentThread = this;
   mOSThread->exitValue = execute(mState->cia, 0, nullptr);
}

uint32_t
Thread::execute(uint32_t address, size_t argc, uint32_t args[])
{
   for (auto i = 0u; i < argc; ++i) {
      mState->gpr[3 + i] = args[i];
   }

   gInterpreter.execute(mState, mState->cia);
   return mState->gpr[3];
}
