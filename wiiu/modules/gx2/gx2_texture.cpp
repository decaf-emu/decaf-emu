#include "gx2.h"
#include "gx2_texture.h"
#include "log.h"

void
GX2InitTextureRegs(GX2Texture *texture)
{
   xLog() << "GX2InitTextureRegs";
}

void
GX2::registerTextureFunctions()
{
   RegisterSystemFunction(GX2InitTextureRegs);
}
