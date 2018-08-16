#include "snduser2.h"
#include "snduser2_axfx.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

BOOL
AXFXReverbHiInit()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiShutdown()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXReverbHiSettings()
{
   decaf_warn_stub();
   return TRUE;
}

BOOL
AXFXMultiChReverbInit()
{
   decaf_warn_stub();
   return TRUE;
}

void
Library::registerAxfxSymbols()
{
   RegisterFunctionExport(AXFXReverbHiInit);
   RegisterFunctionExport(AXFXReverbHiShutdown);
   RegisterFunctionExport(AXFXReverbHiSettings);
   RegisterFunctionExport(AXFXMultiChReverbInit);
}

} // namespace cafe::snduser2
