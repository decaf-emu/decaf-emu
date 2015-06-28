#include "gx2.h"
#include "gx2_temp.h"

uint32_t
GX2TempGetGPUVersion()
{
   return 2;
}

void
GX2::registerTempFunctions()
{
   RegisterSystemFunction(GX2TempGetGPUVersion);
}

