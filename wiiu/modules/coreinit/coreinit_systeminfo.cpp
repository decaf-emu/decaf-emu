#include <ctime>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "coreinit_memory.h"

p32<OSSystemInfo> gSystemInfo;

p32<OSSystemInfo>
OSGetSystemInfo()
{
   return gSystemInfo;
}

void
CoreInit::registerSystemInfoFunctions()
{
   RegisterSystemFunction(OSGetSystemInfo);
}

void
CoreInit::initialiseSystemInformation()
{
   gSystemInfo = OSAllocFromSystem(sizeof(OSSystemInfo), 4);
   gSystemInfo->clockSpeed = CLOCKS_PER_SEC * 4;
}
