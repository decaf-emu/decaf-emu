#include "sysapp.h"
#include "sysapp_title.h"

namespace sysapp
{

uint64_t
SYSGetSystemApplicationTitleId()
{
   decaf_warn_stub();

   return 0x000500101000400Aull;
}

void
Module::registerTitleFunctions()
{
   RegisterKernelFunctionName("_SYSGetSystemApplicationTitleId", SYSGetSystemApplicationTitleId);
}

} // namespace sysapp
