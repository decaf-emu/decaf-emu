#include "tcl.h"
#include "tcl_driver.h"

#include "cafe/libraries/coreinit/coreinit_driver.h"
#include <libcpu/be2_struct.h>

namespace cafe::tcl
{

struct StaticDriverData
{
   be2_val<BOOL> isInitialised;
   be2_val<BOOL> hangWait;
};

static virt_ptr<StaticDriverData>
sDriverData = nullptr;

TCLStatus
TCLGetInfo(virt_ptr<TCLInfo> info)
{
   if (!internal::tclDriverInitialised()) {
      return TCLStatus::NotInitialised;
   }

   info->asicType = TCLAsicType::Unknown5;
   info->chipRevision = TCLChipRevision::Unknown78;
   info->cpMicrocodeVersion = TCLCpMicrocodeVersion::Unknown16;
   info->quadPipes = 4u;
   info->parameterCacheWidth = 16u;
   info->rb = 2u;
   info->addrLibHandle = virt_cast<void *>(virt_addr { 0xF00DBAAD });
   info->sclk = 549999755u;
   return TCLStatus::OK;
}

void
TCLSetHangWait(BOOL hangWait)
{
   sDriverData->hangWait = hangWait;
}

namespace internal
{

void
initialiseTclDriver()
{
   // TODO: OSDriver_Register
   sDriverData->isInitialised = true;
}

bool
tclDriverInitialised()
{
   return sDriverData->isInitialised ? true : false;
}

} // namespace internal

void
Library::registerDriverSymbols()
{
   RegisterFunctionExport(TCLGetInfo);
   RegisterFunctionExport(TCLSetHangWait);

   RegisterDataInternal(sDriverData);
}

} // namespace cafe::tcl
