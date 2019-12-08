#include "nn_spm.h"

namespace cafe::nn_spm
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
   registerExtendedStorageServiceSymbols();
}

} // namespace cafe::nn_spm
