#include "nn_save.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::nn_save
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

   registerLibraryDependency("coreinit");
   registerLibraryDependency("nn_act");
   registerLibraryDependency("nn_acp");

   registerCmdSymbols();
   registerPathSymbols();
}

} // namespace cafe::nn_save
