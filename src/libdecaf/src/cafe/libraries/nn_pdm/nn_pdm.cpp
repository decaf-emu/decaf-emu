#include "nn_pdm.h"

namespace cafe::nn_pdm
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
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
