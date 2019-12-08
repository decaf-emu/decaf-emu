#include "nn_temp.h"

namespace cafe::nn_temp
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

   registerTempDirSymbols();
}

} // namespace cafe::nn_temp
