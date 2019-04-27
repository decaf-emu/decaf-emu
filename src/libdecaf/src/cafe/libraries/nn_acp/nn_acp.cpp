#include "nn_acp.h"
#include "nn_acp_internal_driver.h"

#include "cafe/libraries/coreinit/coreinit_dynload.h"
#include "cafe/libraries/cafe_hle.h"

using namespace cafe::coreinit;

namespace cafe::nn_acp
{

static int32_t
rpl_entry(OSDynLoad_ModuleHandle moduleHandle,
          OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);

   if (reason == OSDynLoad_EntryReason::Loaded) {
      internal::startDriver(moduleHandle);
   } else if (reason == OSDynLoad_EntryReason::Unloaded) {
      internal::stopDriver(moduleHandle);
   }

   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerClientSymbols();
   registerDeviceSymbols();
   registerDriverSymbols();
   registerMiscServiceSymbols();
   registerSaveServiceSymbols();
}

} // namespace cafe::nn_acp
