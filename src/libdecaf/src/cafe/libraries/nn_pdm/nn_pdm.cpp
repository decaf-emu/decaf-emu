#include "nn_pdm.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::nn_pdm
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerClientSymbols();
   registerCosServiceSymbols();
}

} // namespace cafe::nn_pdm
