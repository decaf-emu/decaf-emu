#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/structsize.h"
#include "gpu/latte_registers.h"

#pragma pack(push, 1)

struct GX2Sampler
{
   struct {
      be_val<latte::SQ_TEX_SAMPLER_WORD0_N> word0;
      be_val<latte::SQ_TEX_SAMPLER_WORD1_N> word1;
      be_val<latte::SQ_TEX_SAMPLER_WORD2_N> word2;
   } regs;
};
CHECK_SIZE(GX2Sampler, 12);

#pragma pack(pop)

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode clampMode,
               GX2TexXYFilterMode minMagFilterMode);

void
GX2InitSamplerBorderType(GX2Sampler *sampler,
                         GX2TexBorderType borderType);

void
GX2InitSamplerClamping(GX2Sampler *sampler,
                       GX2TexClampMode clampX,
                       GX2TexClampMode clampY,
                       GX2TexClampMode clampZ);

void
GX2InitSamplerDepthCompare(GX2Sampler *sampler,
                           GX2CompareFunction depthCompare);

void
GX2InitSamplerFilterAdjust(GX2Sampler *sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode perfMip,
                           GX2TexZPerfMode perfZ);

void
GX2InitSamplerLOD(GX2Sampler *sampler, float lodMin, float lodMax, float lodBias);

void
GX2InitSamplerLODAdjust(GX2Sampler *sampler, float unk1, BOOL unk2);

void
GX2InitSamplerRoundingMode(GX2Sampler *sampler,
                           GX2RoundingMode roundingMode);

void
GX2InitSamplerXYFilter(GX2Sampler *sampler,
                       GX2TexXYFilterMode filterMag,
                       GX2TexXYFilterMode filterMin,
                       GX2TexAnisoRatio maxAniso);

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
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
