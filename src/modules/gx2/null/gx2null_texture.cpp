#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/gx2/gx2_texture.h"
#include "utils/log.h"

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
