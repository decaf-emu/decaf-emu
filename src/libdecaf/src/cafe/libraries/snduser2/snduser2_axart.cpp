#include "snduser2.h"
#include "snduser2_axart.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

void
AXARTServiceSounds()
{
   decaf_warn_stub();
}

void
Library::registerAxArtSymbols()
{
   RegisterFunctionExport(AXARTServiceSounds);
}

} // namespace cafe::snduser2
