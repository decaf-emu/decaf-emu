#include "sysapp.h"
#include "sysapp_title.h"

uint64_t
SYSGetSystemApplicationTitleId()
{
   // TODO: Find real value
   return 0xFFFFFFFF10101010ull;
}

void
SysApp::registerTitleFunctions()
{
   RegisterKernelFunctionName("_SYSGetSystemApplicationTitleId", SYSGetSystemApplicationTitleId);
}
