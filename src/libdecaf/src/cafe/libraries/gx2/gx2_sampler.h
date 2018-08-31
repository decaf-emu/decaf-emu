#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2_sampler Sampler
 * \ingroup gx2
 * @{
 */

#pragma pack(push, 1)

struct GX2Sampler
{
   struct {
      be2_val<latte::SQ_TEX_SAMPLER_WORD0_N> word0;
      be2_val<latte::SQ_TEX_SAMPLER_WORD1_N> word1;
      be2_val<latte::SQ_TEX_SAMPLER_WORD2_N> word2;
   } regs;
};
CHECK_OFFSET(GX2Sampler, 0x00, regs.word0);
CHECK_OFFSET(GX2Sampler, 0x04, regs.word1);
CHECK_OFFSET(GX2Sampler, 0x08, regs.word2);
CHECK_SIZE(GX2Sampler, 0x0C);

#pragma pack(pop)

void
GX2InitSampler(virt_ptr<GX2Sampler> sampler,
               GX2TexClampMode clampMode,
               GX2TexXYFilterMode minMagFilterMode);

void
GX2InitSamplerBorderType(virt_ptr<GX2Sampler> sampler,
                         GX2TexBorderType borderType);

void
GX2InitSamplerClamping(virt_ptr<GX2Sampler> sampler,
                       GX2TexClampMode clampX,
                       GX2TexClampMode clampY,
                       GX2TexClampMode clampZ);

void
GX2InitSamplerDepthCompare(virt_ptr<GX2Sampler> sampler,
                           GX2CompareFunction depthCompare);

void
GX2InitSamplerFilterAdjust(virt_ptr<GX2Sampler> sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode perfMip,
                           GX2TexZPerfMode perfZ);

void
GX2InitSamplerLOD(virt_ptr<GX2Sampler> sampler,
                  float lodMin,
                  float lodMax,
                  float lodBias);

void
GX2InitSamplerLODAdjust(virt_ptr<GX2Sampler> sampler,
                        float unk1,
                        BOOL unk2);

void
GX2InitSamplerRoundingMode(virt_ptr<GX2Sampler> sampler,
                           GX2RoundingMode roundingMode);

void
GX2InitSamplerXYFilter(virt_ptr<GX2Sampler> sampler,
                       GX2TexXYFilterMode filterMag,
                       GX2TexXYFilterMode filterMin,
                       GX2TexAnisoRatio maxAniso);

void
GX2InitSamplerZMFilter(virt_ptr<GX2Sampler> sampler,
                       GX2TexZFilterMode filterZ,
                       GX2TexMipFilterMode filterMip);

void
GX2SetPixelSamplerBorderColor(uint32_t unit,
                              float red,
                              float green,
                              float blue,
                              float alpha);
void
GX2SetVertexSamplerBorderColor(uint32_t unit,
                               float red,
                               float green,
                               float blue,
                               float alpha);
void
GX2SetGeometrySamplerBorderColor(uint32_t unit,
                                 float red,
                                 float green,
                                 float blue,
                                 float alpha);

/** @} */

} // namespace cafe::gx2
