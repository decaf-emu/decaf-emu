#include "gx2.h"

GX2::GX2()
{
   registerCoreFunctions();
   registerDisplayFunctions();
   registerDisplayListFunctions();
   registerRenderStateFunctions();
   registerShaderFunctions();
   registerTextureFunctions();
}

void GX2::initialise()
{
}
