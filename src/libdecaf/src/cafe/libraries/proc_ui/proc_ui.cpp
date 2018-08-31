#include "proc_ui.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::proc_ui
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

   registerMessagesFunctions();
}

} // namespace cafe::proc_ui
