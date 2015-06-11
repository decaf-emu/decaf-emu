#include "coreinit.h"
#include "coreinit_time.h"
#include <ctime>

// TODO: Emulate time? Update time base registers?

OSTime
OSGetSystemTime()
{
   return clock();
}

void
CoreInit::registerTimeFunctions()
{
   RegisterSystemFunction(OSGetSystemTime);
}
