#include "gx2.h"

GX2::GX2()
{
}

void GX2::initialise()
{
}

void GX2::RegisterFunctions()
{
   registerContextFunctions();
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
