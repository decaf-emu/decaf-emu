#pragma once
#include "types.h"
#include "gpu/latte_registers.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
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
   be_val<uint32_t> compMap;
   latte::SQ_TEX_RESOURCE_WORD0_0 regWord0;
   latte::SQ_TEX_RESOURCE_WORD1_0 regWord1;
   latte::SQ_TEX_RESOURCE_WORD4_0 regWord4;
   latte::SQ_TEX_RESOURCE_WORD5_0 regWord5;
   latte::SQ_TEX_RESOURCE_WORD6_0 regWord6;
};
CHECK_OFFSET(GX2Texture, 0x0, surface);
CHECK_OFFSET(GX2Texture, 0x74, viewFirstMip);
CHECK_OFFSET(GX2Texture, 0x78, viewNumMips);
CHECK_OFFSET(GX2Texture, 0x7c, viewFirstSlice);
CHECK_OFFSET(GX2Texture, 0x80, viewNumSlices);
CHECK_OFFSET(GX2Texture, 0x84, compMap);
CHECK_OFFSET(GX2Texture, 0x88, regWord0);
CHECK_OFFSET(GX2Texture, 0x8C, regWord1);
CHECK_OFFSET(GX2Texture, 0x90, regWord4);
CHECK_OFFSET(GX2Texture, 0x94, regWord5);
CHECK_OFFSET(GX2Texture, 0x98, regWord6);
CHECK_SIZE(GX2Texture, 0x9c);

#pragma pack(pop)

void
GX2InitTextureRegs(GX2Texture *texture);

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit);
