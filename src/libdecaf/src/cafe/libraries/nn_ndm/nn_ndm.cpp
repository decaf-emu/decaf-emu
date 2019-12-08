#include "nn_ndm.h"

namespace cafe::nn_ndm
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

   registerNdmSymbols();
}

} // namespace cafe::nn_ndm
