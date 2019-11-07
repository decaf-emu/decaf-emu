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

int32_t
SYSGetLauncherArgs(virt_ptr<void> args)
{
   decaf_warn_stub();
   std::memset(args.get(), 0, 0xC);
   return 1;
}

int32_t
SYSGetStandardResult(virt_ptr<uint32_t> arg1,
                     uint32_t arg2,
                     uint32_t arg3)
{
   decaf_warn_stub();
   std::memset(arg1.get(), 0, 0x4);
   return 1;
}

void
Library::registerCallerArgsSymbols()
{
   RegisterFunctionExport(SYSGetCallerPFID);
   RegisterFunctionExport(SYSGetCallerTitleId);
   RegisterFunctionExport(SYSGetCallerUPID);
   RegisterFunctionExport(SYSGetStandardResult);
   RegisterFunctionExportName("_SYSGetLauncherArgs", SYSGetLauncherArgs);
}

} // namespace cafe::sysapp
