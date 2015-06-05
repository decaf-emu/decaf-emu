#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "memory.h"

p32<OSSystemInfo> gSystemInfo;

p32<OSSystemInfo>
OSGetSystemInfo()
{
   return make_p32<OSSystemInfo>(nullptr);
}

void
CoreInit::registerSystemInfoFunctions()
{
   gSystemInfo = make_p32<OSSystemInfo>(gMemory.alloc(MemoryType::SystemData, sizeof(OSSystemInfo)));
   RegisterSystemFunction(OSGetSystemInfo);
}
