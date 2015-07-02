#pragma once
#include <string>
#include <thread>
#include "ppc.h"
#include "modules/coreinit/coreinit_thread.h"

class Interpreter;
class System;
struct ThreadState;

class Thread
{
public:
   Thread(OSThread *thread, ThreadEntryPoint entry,
          uint32_t argc, void *argv,
          uint8_t *stack, uint32_t stackSize,
          uint32_t priority, OSThreadAttributes::Flags attributes);
   ~Thread();

   uint32_t getCoreID() const;
   ThreadState *getThreadState() const;
   OSThread *getOSThread() const;

   uint32_t join();
   void resume();
   bool run(ThreadEntryPoint entry, uint32_t argc, p32<void> argv);
   uint32_t execute(uint32_t address, size_t argc, uint32_t args[]);

private:
   void entry();

public:
   static Thread *getCurrentThread();

private:
   uint32_t mCoreID;
   std::string mName;
   uint32_t mStackBase;
   uint32_t mStackSize;
   OSThread *mOSThread;

   std::aligned_storage<sizeof(ThreadState), 16>::type mAlignedState;
   ThreadState *mState;
   std::thread mThread;
};
