#include "gx2.h"
#include "gx2_texture.h"
#include "log.h"

void
GX2InitTextureRegs(GX2Texture *texture)
{
}

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit)
{
}

void
GX2::registerTextureFunctions()
{
   RegisterKernelFunction(GX2InitTextureRegs);
   RegisterKernelFunction(GX2SetPixelTexture);
}
