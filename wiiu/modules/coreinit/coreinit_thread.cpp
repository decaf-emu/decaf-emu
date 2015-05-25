#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/thread.h"

struct OSThread;

OSThread* OSGetCurrentThread()
{
   return nullptr;
}

void CoreInit::registerThreadFunctions()
{
   RegisterSystemFunction(OSGetCurrentThread);
}
