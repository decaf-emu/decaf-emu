#include "nn_act.h"

namespace cafe::nn_act
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
   registerAccountLoaderServiceSymbols();
   registerAccountManagerServiceSymbols();
   registerClientStandardServiceSymbols();
   registerServerStandardServiceSymbols();
}

} // namespace cafe::nn_act
