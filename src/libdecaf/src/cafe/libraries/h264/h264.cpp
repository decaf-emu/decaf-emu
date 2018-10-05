#include "h264.h"
#include "cafe/libraries/coreinit/coreinit_dynload.h"

namespace cafe::h264
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

   registerDecSymbols();
}

} // namespace cafe::h264
