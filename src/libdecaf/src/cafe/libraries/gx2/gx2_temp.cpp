#include "gx2.h"
#include "gx2_temp.h"

namespace cafe::gx2
{

uint32_t
GX2TempGetGPUVersion()
{
   return 2;
}

void
Library::registerTempSymbols()
{
   RegisterFunctionExport(GX2TempGetGPUVersion);
}

} // namespace cafe::gx2
