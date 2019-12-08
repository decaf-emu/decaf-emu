#include "dmae.h"
#include "dmae_ring.h"

namespace cafe::dmae
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
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
