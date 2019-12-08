#include "nn_save.h"

namespace cafe::nn_save
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

   registerLibraryDependency("coreinit");
   registerLibraryDependency("nn_act");
   registerLibraryDependency("nn_acp");

   registerCmdSymbols();
   registerPathSymbols();
}

} // namespace cafe::nn_save
