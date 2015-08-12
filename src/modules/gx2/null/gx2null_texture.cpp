#include "../gx2.h"
#ifdef GX2_NULL

#include "../gx2_texture.h"
#include "log.h"

void
GX2InitSampler(GX2Sampler *sampler,
   GX2TexClampMode::Mode clampMode,
   GX2TexXYFilterMode::Mode minMagFilterMode)
{
}

void
GX2InitTextureRegs(GX2Texture *texture)
{
}

void
GX2SetPixelTexture(GX2Texture *texture,
   uint32_t unit)
{
}

#endif
