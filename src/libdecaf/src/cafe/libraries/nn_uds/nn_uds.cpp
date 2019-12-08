#include "nn_uds.h"

namespace cafe::nn_uds
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

   registerApiSymbols();
}

} // namespace cafe::nn_uds
