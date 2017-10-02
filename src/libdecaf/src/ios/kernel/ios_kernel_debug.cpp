#include "ios_kernel_debug.h"
#include "ios_kernel_process.h"

namespace ios::kernel
{

static SecurityLevel
sSecurityLevel = SecurityLevel::Normal;

Error
IOS_SetSecurityLevel(SecurityLevel level)
{
   if (IOS_GetCurrentProcessId() != ProcessId::MCP) {
      return Error::Access;
   }

   sSecurityLevel = level;
   return Error::OK;
}

SecurityLevel
IOS_GetSecurityLevel()
{
   return sSecurityLevel;
}

namespace internal
{

void
setSecurityLevel(SecurityLevel level)
{
   sSecurityLevel = level;
}

} // namespace internal

} // namespace ios::kernel
