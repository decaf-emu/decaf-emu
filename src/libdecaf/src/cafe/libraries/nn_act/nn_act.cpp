#include "nn_act.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::nn_act
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

   registerLibSymbols();
}

} // namespace cafe::nn_act
