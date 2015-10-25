#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_texture.h"
#include "dx12_state.h"
#include "dx12_texture.h"

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
_GX2SetPixelTexture(GX2Texture *texture,
                   uint32_t unit)
{
   DXTextureData *textureData = dx::getTexture(texture);
   textureData->upload();
   gDX.commandList->SetGraphicsRootDescriptorTable(1 + unit, *textureData->srv);
}

void
GX2SetPixelTexture(GX2Texture *texture,
   uint32_t unit)
{
   DX_DLCALL(_GX2SetPixelTexture, texture, unit);
}

#endif
