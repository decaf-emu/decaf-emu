#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_debug.h"
#include "modules/gx2/gx2_texture.h"
#include "dx12_state.h"
#include "dx12_texture.h"

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode::Value clampMode,
               GX2TexXYFilterMode::Value minMagFilterMode)
{
   // TODO: GX2InitSampler
}

void
GX2InitSamplerLOD(GX2Sampler *sampler, float unk1, float unk2, float unk3)
{
   // TODO: GX2InitSamplerLOD
}

void
GX2InitSamplerZMFilter(GX2Sampler *samper, uint32_t unk1, uint32_t unk2)
{
   // TODO: GX2InitSamplerZMFilter
}

void
GX2InitSamplerClamping(GX2Sampler *sampler, uint32_t unk1, uint32_t unk2, uint32_t unk3)
{
   // TODO: GX2InitSamplerClamping
}

void
GX2InitSamplerXYFilter(GX2Sampler *sampler, uint32_t unk1, uint32_t unk2, uint32_t unk3)
{
   // TODO: GX2InitSamplerXYFilter
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
   gLog->debug("_GX2SetPixelTexture({}, {})", memory_untranslate(texture), unit);

   GX2DumpTexture(texture);
   DXTextureData *textureData = dx::getTexture(texture);
   textureData->upload();
   
   gDX.activeTextures[unit] = textureData;
}

void
GX2SetPixelTexture(GX2Texture *texture,
   uint32_t unit)
{
   DX_DLCALL(_GX2SetPixelTexture, texture, unit);
}

#endif
