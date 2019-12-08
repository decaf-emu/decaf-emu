#include "nn_sl.h"

namespace cafe::nn_sl
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

   registerDrcTransferrerSymbols();
   registerLibSymbols();
}

} // namespace cafe::nn_sl
