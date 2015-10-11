#include "../gx2.h"
#ifdef GX2_DX12

#include "../gx2_texture.h"
#include "../gx2_debug.h"
#include "log.h"

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode::Mode clampMode,
               GX2TexXYFilterMode::Mode minMagFilterMode)
{
   // TODO: GX2InitSampler
}

void
GX2InitTextureRegs(GX2Texture *texture)
{
   // TODO: GX2InitTextureRegs
}

void
GX2SetPixelTexture(GX2Texture *texture,
                   uint32_t unit)
{
   // TODO: GX2SetPixelTexture
   GX2DumpTexture(texture);
}

#endif
