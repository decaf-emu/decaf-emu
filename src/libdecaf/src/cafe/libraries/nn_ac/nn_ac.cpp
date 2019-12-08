#include "nn_ac.h"

namespace cafe::nn_ac
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

   registerCApiFunctions();
   registerLibFunctions();
}

} // namespace cafe::nn_ac
