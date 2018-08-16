#include "dmae.h"
#include "dmae_ring.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::dmae
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   DMAEInit();
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerRingSymbols();
}

} // namespace cafe::coreinit
