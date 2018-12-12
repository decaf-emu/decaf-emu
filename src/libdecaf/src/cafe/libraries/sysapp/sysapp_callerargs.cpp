#include "sysapp.h"
#include "sysapp_callerargs.h"

#include <cafe/libraries/cafe_hle_stub.h>

namespace cafe::sysapp
{

uint32_t
SYSGetCallerPFID()
{
   return SYSGetCallerUPID();
}

uint64_t
SYSGetCallerTitleId()
{
   decaf_warn_stub();
   return 0ull;
}

uint32_t
SYSGetCallerUPID()
{
   decaf_warn_stub();
   return 0u;
}

void
Library::registerCallerArgsSymbols()
{
   RegisterFunctionExport(SYSGetCallerPFID);
   RegisterFunctionExport(SYSGetCallerTitleId);
   RegisterFunctionExport(SYSGetCallerUPID);
}

} // namespace cafe::sysapp
