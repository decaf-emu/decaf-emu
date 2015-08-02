#include "gx2.h"

GX2::GX2()
{
}

void GX2::initialise()
{
}

void GX2::RegisterFunctions()
{
   registerCoreFunctions();
   registerDisplayFunctions();
   registerDisplayListFunctions();
   registerDrawFunctions();
   registerRenderStateFunctions();
   registerResourceFunctions();
   registerShaderFunctions();
   registerSurfaceFunctions();
   registerTempFunctions();
   registerTextureFunctions();
}
