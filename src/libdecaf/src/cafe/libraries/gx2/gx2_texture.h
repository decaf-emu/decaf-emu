#pragma once
#include "gx2_enum.h"
#include "gx2_surface.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_texture Texture
 * \ingroup gx2
 * @{
 */

struct GX2Sampler;

#pragma pack(push, 1)

struct GX2Texture
{
   be2_struct<GX2Surface> surface;
   be2_val<uint32_t> viewFirstMip;
   be2_val<uint32_t> viewNumMips;
   be2_val<uint32_t> viewFirstSlice;
   be2_val<uint32_t> viewNumSlices;
   be2_val<uint32_t> compMap;

   struct
   {
      be2_val<latte::SQ_TEX_RESOURCE_WORD0_N> word0;
      be2_val<latte::SQ_TEX_RESOURCE_WORD1_N> word1;
      be2_val<latte::SQ_TEX_RESOURCE_WORD4_N> word4;
      be2_val<latte::SQ_TEX_RESOURCE_WORD5_N> word5;
      be2_val<latte::SQ_TEX_RESOURCE_WORD6_N> word6;
   } regs;
};
CHECK_OFFSET(GX2Texture, 0x00, surface);
CHECK_OFFSET(GX2Texture, 0x74, viewFirstMip);
CHECK_OFFSET(GX2Texture, 0x78, viewNumMips);
CHECK_OFFSET(GX2Texture, 0x7C, viewFirstSlice);
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
GX2InitTextureRegs(virt_ptr<GX2Texture> texture);

void
GX2SetPixelTexture(virt_ptr<GX2Texture> texture,
                   uint32_t unit);

void
GX2SetVertexTexture(virt_ptr<GX2Texture> texture,
                    uint32_t unit);

void
GX2SetGeometryTexture(virt_ptr<GX2Texture> texture,
                      uint32_t unit);

/** @} */

} // namespace cafe::gx2
