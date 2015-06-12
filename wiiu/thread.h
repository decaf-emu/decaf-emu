#pragma once
#include <string>
#include "ppc.h"
#include "modules/coreinit/coreinit_thread.h"

class Interpreter;
class System;
struct ThreadState;

class Thread
{
public:
   Thread(System *system, uint32_t stackSize, uint32_t entryPoint);
   ~Thread();

   void run(Interpreter &interp);

   System *getSystem() const;
   ThreadState *getThreadState() const;
   p32<OSThread> getOSThread() const;

public:
   static Thread *getCurrentThread();

private:
   std::string mName;
   System *mSystem;
   uint32_t mStackBase;
   uint32_t mStackSize;
   p32<OSThread> mOSThread;

   std::aligned_storage<sizeof(ThreadState), 16>::type mAlignedState;
   ThreadState *mState;
};
