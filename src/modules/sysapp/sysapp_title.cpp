#include "sysapp.h"
#include "sysapp_title.h"

namespace sysapp
{

uint64_t
SYSGetSystemApplicationTitleId()
{
   // TODO: Find real value
   return 0xFFFFFFFF10101010ull;
}

void
Module::registerTitleFunctions()
{
   RegisterKernelFunctionName("_SYSGetSystemApplicationTitleId", SYSGetSystemApplicationTitleId);
}

} // namespace sysapp
