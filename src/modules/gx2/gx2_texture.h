#pragma once
#include "systemtypes.h"
#include "gx2_surface.h"

// Thanks to Nintendo for nice debug messages giving us variables ! :)

#pragma pack(push, 1)

// GX2InitTextureRegs
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
GX2InitTextureRegs(GX2Texture *texture);
