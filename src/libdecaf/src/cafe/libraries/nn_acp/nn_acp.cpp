#include "nn_acp.h"
#include "nn_acp_internal_driver.h"

#include "cafe/libraries/coreinit/coreinit_dynload.h"
#include "cafe/libraries/cafe_hle.h"

using namespace cafe::coreinit;

namespace cafe::nn_acp
{

struct PrologueToken
{
   uint32_t lr;
};

static PrologueToken
hleEntryPrologue()
{
   auto core = cpu::this_core::state();

   // Save state
   auto token = PrologueToken { };
   token.lr = core->lr;

   // Modify registers
   core->lr = core->nia;
   return token;
}

static void
hleEntryEpilogue(const PrologueToken &token)
{
   auto core = cpu::this_core::state();

   // Restore state
   core->lr = token.lr;
}

static int32_t
rpl_entry(OSDynLoad_ModuleHandle moduleHandle,
          OSDynLoad_EntryReason reason)
{
   coreinit::internal::relocateHleLibrary(moduleHandle);
   auto prologueToken = hleEntryPrologue();

   if (reason == OSDynLoad_EntryReason::Loaded) {
      internal::startDriver(moduleHandle);
   } else if (reason == OSDynLoad_EntryReason::Unloaded) {
      internal::stopDriver(moduleHandle);
   }

   hleEntryEpilogue(prologueToken);
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerClientSymbols();
   registerDeviceSymbols();
   registerDriverSymbols();
   registerTitleSymbols();
}

} // namespace cafe::nn_acp
