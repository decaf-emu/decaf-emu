#pragma once
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "types.h"
#include "utils/be_val.h"
#include "utils/structsize.h"

struct GX2Sampler;

#pragma pack(push, 1)

struct GX2Texture
{
   GX2Surface surface;
   be_val<uint32_t> viewFirstMip;
   be_val<uint32_t> viewNumMips;
   be_val<uint32_t> viewFirstSlice;
   be_val<uint32_t> viewNumSlices;
   UNKNOWN(24);
};
CHECK_OFFSET(GX2Texture, 0x0, surface);
CHECK_OFFSET(GX2Texture, 0x74, viewFirstMip);
CHECK_OFFSET(GX2Texture, 0x78, viewNumMips);
CHECK_OFFSET(GX2Texture, 0x7c, viewFirstSlice);
CHECK_OFFSET(GX2Texture, 0x80, viewNumSlices);
CHECK_SIZE(GX2Texture, 0x9c);

#pragma pack(pop)

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode::Value clampMode,
               GX2TexXYFilterMode::Value minMagFilterMode);

void
GX2InitSamplerLOD(GX2Sampler *sampler, float unk1, float unk2, float unk3);

void
GX2InitSamplerZMFilter(GX2Sampler *samper, uint32_t unk1, uint32_t unk2);

void
GX2InitSamplerClamping(GX2Sampler *sampler, uint32_t unk1, uint32_t unk2, uint32_t unk3);

void
GX2InitSamplerXYFilter(GX2Sampler *sampler, uint32_t unk1, uint32_t unk2, uint32_t unk3);

void
GX2InitTextureRegs(GX2Texture *texture);

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit);
