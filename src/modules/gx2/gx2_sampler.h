#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/structsize.h"
#include "gpu/latte_registers.h"

#pragma pack(push, 1)

struct GX2Sampler
{
   struct {
      be_val<latte::SQ_TEX_SAMPLER_WORD0_0> word0;
      be_val<latte::SQ_TEX_SAMPLER_WORD1_0> word1;
      be_val<latte::SQ_TEX_SAMPLER_WORD2_0> word2;
   } regs;
};
CHECK_SIZE(GX2Sampler, 12);

#pragma pack(pop)

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode::Value clampMode,
               GX2TexXYFilterMode::Value minMagFilterMode);

void
GX2InitSamplerBorderType(GX2Sampler *sampler,
                         GX2TexBorderType::Value borderType);

void
GX2InitSamplerClamping(GX2Sampler *sampler,
                       GX2TexClampMode::Value clampX,
                       GX2TexClampMode::Value clampY,
                       GX2TexClampMode::Value clampZ);

void
GX2InitSamplerDepthCompare(GX2Sampler *sampler,
                           GX2CompareFunction::Value depthCompare);

void
GX2InitSamplerFilterAdjust(GX2Sampler *sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode::Value perfMip,
                           GX2TexZPerfMode::Value perfZ);

void
GX2InitSamplerLOD(GX2Sampler *sampler, float lodMin, float lodMax, float lodBias);

void
GX2InitSamplerLODAdjust(GX2Sampler *sampler, float unk1, BOOL unk2);

void
GX2InitSamplerRoundingMode(GX2Sampler *sampler,
                           GX2RoundingMode::Value roundingMode);

void
GX2InitSamplerXYFilter(GX2Sampler *sampler,
                       GX2TexXYFilterMode::Value filterMag,
                       GX2TexXYFilterMode::Value filterMin,
                       GX2TexAnisoRatio::Value maxAniso);

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
                       GX2TexZFilterMode::Value filterZ,
                       GX2TexMipFilterMode::Value filterMip);