#include "nsysnet.h"
#include "nsysnet_socket_lib.h"
#include "nsysnet_nssl.h"

namespace cafe::nsysnet
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

   registerEndianSymbols();
   registerSocketLibSymbols();
   registerSslSymbols();
}

} // namespace cafe::nsysnet
