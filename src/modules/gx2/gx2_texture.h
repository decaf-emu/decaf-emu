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

   struct
   {
      be_val<latte::SQ_TEX_RESOURCE_WORD0_0> word0;
      be_val<latte::SQ_TEX_RESOURCE_WORD1_0> word1;
      be_val<latte::SQ_TEX_RESOURCE_WORD4_0> word4;
      be_val<latte::SQ_TEX_RESOURCE_WORD5_0> word5;
      be_val<latte::SQ_TEX_RESOURCE_WORD6_0> word6;
   } regs;
};
CHECK_OFFSET(GX2Texture, 0x0, surface);
CHECK_OFFSET(GX2Texture, 0x74, viewFirstMip);
CHECK_OFFSET(GX2Texture, 0x78, viewNumMips);
CHECK_OFFSET(GX2Texture, 0x7c, viewFirstSlice);
CHECK_OFFSET(GX2Texture, 0x80, viewNumSlices);
CHECK_OFFSET(GX2Texture, 0x84, compMap);
CHECK_OFFSET(GX2Texture, 0x88, regs.word0);
CHECK_OFFSET(GX2Texture, 0x8C, regs.word1);
CHECK_OFFSET(GX2Texture, 0x90, regs.word4);
CHECK_OFFSET(GX2Texture, 0x94, regs.word5);
CHECK_OFFSET(GX2Texture, 0x98, regs.word6);
CHECK_SIZE(GX2Texture, 0x9c);

#pragma pack(pop)

void
GX2InitTextureRegs(GX2Texture *texture);

void
GX2SetPixelTexture(GX2Texture *texture, uint32_t unit);
