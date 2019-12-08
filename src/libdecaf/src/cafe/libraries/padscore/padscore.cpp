#include "padscore.h"

namespace cafe::padscore
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

   registerKpadSymbols();
   registerWpadSymbols();
}

} // namespace cafe::padscore
