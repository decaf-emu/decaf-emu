#include "sndcore2.h"
#include "sndcore2_mix.h"
#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::sndcore2
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

} // namespace cafe::sndcore2
