#include <ctime>
#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "memory.h"

p32<OSSystemInfo> gSystemInfo;

p32<OSSystemInfo>
OSGetSystemInfo()
{
   return gSystemInfo;
}

void
CoreInit::registerSystemInfoFunctions()
{
   gSystemInfo = make_p32<OSSystemInfo>(gMemory.alloc(MemoryType::SystemData, sizeof(OSSystemInfo)));
   gSystemInfo->clockSpeed = CLOCKS_PER_SEC * 4;
   RegisterSystemFunction(OSGetSystemInfo);
}
