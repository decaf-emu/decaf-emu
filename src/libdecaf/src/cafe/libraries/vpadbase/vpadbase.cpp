#include "vpadbase.h"

namespace cafe::vpadbase
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

   registerControllerSymbols();
}

} // namespace cafe::vpadbase
