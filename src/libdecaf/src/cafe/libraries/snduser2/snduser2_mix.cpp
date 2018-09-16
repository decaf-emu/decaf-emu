#include "snduser2.h"
#include "snduser2_mix.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::snduser2
{

void
MIXUpdateSettings()
{
   decaf_warn_stub();
}

void
Library::registerMixSymbols()
{
   RegisterFunctionExport(MIXUpdateSettings);
}

} // namespace cafe::snduser2
