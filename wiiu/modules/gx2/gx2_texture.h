#pragma once
#include "systemtypes.h"

// Thanks to Nintendo for nice debug messages giving us variables ! :)

#pragma pack(push, 1)

// GX2InitTextureRegs
// G2XRCreateSurface
struct GX2Surface
{
   be_val<uint32_t> dim; // "GX2_SURFACE_DIM_2D_MSAA or GX2_SURFACE_DIM_2D_MSAA_ARRAY" = 0 or 6?
   UNKNOWN(0x10);
   be_val<uint32_t> format;
   be_val<uint32_t> aa;
   union // Is this correct?? Union???
   {
      be_val<uint32_t> use; // GX2InitTextureRegs
      be_val<uint32_t> resourceFlags; // G2XRCreateSurface
   };
   be_val<uint32_t> imageSize;
   be_ptr<void> imagePtr;
   UNKNOWN(0x4);
   be_ptr<void> mipPtr;
   be_val<uint32_t> tileMode;
   UNKNOWN(0x4);
   be_val<uint32_t> alignment;
   be_val<uint32_t> pitch;
   UNKNOWN(0x74 - 0x40);
};
CHECK_OFFSET(GX2Surface, 0x0, dim);
CHECK_OFFSET(GX2Surface, 0x14, format);
CHECK_OFFSET(GX2Surface, 0x18, aa);
CHECK_OFFSET(GX2Surface, 0x1c, use);
CHECK_OFFSET(GX2Surface, 0x1c, resourceFlags);
CHECK_OFFSET(GX2Surface, 0x20, imageSize);
CHECK_OFFSET(GX2Surface, 0x24, imagePtr);
CHECK_OFFSET(GX2Surface, 0x2c, mipPtr);
CHECK_OFFSET(GX2Surface, 0x30, tileMode);
CHECK_OFFSET(GX2Surface, 0x38, alignment);
CHECK_OFFSET(GX2Surface, 0x3C, pitch);
CHECK_SIZE(GX2Surface, 0x74);

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
