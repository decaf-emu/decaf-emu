#pragma once
#include <string>
#include "ppc.h"
#include "modules/coreinit/coreinit_thread.h"

class SystemThread
{
public:
   SystemThread(struct Module *module, uint32_t stackSize, uint32_t entryPoint);
   ~SystemThread();

   void run(class Interpreter &interp);

   ThreadState *getThreadState();
   p32<OSThread> getOSThread();

public:
   static SystemThread *getCurrentThread();

private:
   std::string mName;
   ThreadState mState;
   p32<OSThread> mOSThread;
   uint32_t mStackBase;
   uint32_t mStackSize;
};
