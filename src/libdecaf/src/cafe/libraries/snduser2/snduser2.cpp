#include "snduser2.h"
#include "snduser2_axfx_hooks.h"

namespace cafe::snduser2
{

static int32_t
rpl_entry(coreinit::OSDynLoad_ModuleHandle moduleHandle,
          coreinit::OSDynLoad_EntryReason reason)
{
   internal::initialiseAxfxHooks();
   return 0;
}

void
Library::registerSymbols()
{
   RegisterEntryPoint(rpl_entry);

   registerAxArtSymbols();
   registerAxfxChorusSymbols();
   registerAxfxChorusExpSymbols();
   registerAxfxDelaySymbols();
   registerAxfxDelayExpSymbols();
   registerAxfxHooksSymbols();
   registerAxfxMultiChReverbSymbols();
   registerAxfxReverbHiSymbols();
   registerAxfxReverbHiExpSymbols();
   registerAxfxReverbStdSymbols();
   registerAxfxReverbStdExpSymbols();
   registerMixSymbols();
   registerSpSymbols();
}

} // namespace cafe::snduser2
