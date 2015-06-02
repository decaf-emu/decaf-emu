#include "coreinit.h"
#include "coreinit_systeminfo.h"
#include "memory.h"

// OSSystemInfo has to be in virtual memory!!!!
p32<OSSystemInfo> gSystemInfo;

p32<OSSystemInfo>
OSGetSystemInfo()
{
   return make_p32<OSSystemInfo>(nullptr);
}

void
CoreInit::registerSystemInfoFunctions()
{
   RegisterSystemFunction(OSGetSystemInfo);
}
