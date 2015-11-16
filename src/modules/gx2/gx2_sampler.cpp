#include "gx2.h"
#include "gx2_sampler.h"

static inline uint32_t
floatToFixedPoint(float value, uint32_t bits, float min, float max)
{
   return static_cast<uint32_t>((value - min) * (static_cast<float>(1 << bits) / (max - min)));
}

void
GX2InitSampler(GX2Sampler *sampler,
               GX2TexClampMode::Value clampMode,
               GX2TexXYFilterMode::Value minMagFilterMode)
{
   sampler->regs.word0.CLAMP_X = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   sampler->regs.word0.CLAMP_Y = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   sampler->regs.word0.CLAMP_Z = static_cast<latte::SQ_TEX_CLAMP>(clampMode);
   sampler->regs.word0.XY_MAG_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode);
   sampler->regs.word0.XY_MIN_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode);
}

void
GX2InitSamplerBorderType(GX2Sampler *sampler,
                         GX2TexBorderType::Value borderType)
{
   sampler->regs.word0.BORDER_COLOR_TYPE = static_cast<latte::SQ_TEX_BORDER_COLOR>(borderType);
}

void
GX2InitSamplerClamping(GX2Sampler *sampler,
                       GX2TexClampMode::Value clampX,
                       GX2TexClampMode::Value clampY,
                       GX2TexClampMode::Value clampZ)
{
   sampler->regs.word0.CLAMP_X = static_cast<latte::SQ_TEX_CLAMP>(clampX);
   sampler->regs.word0.CLAMP_Y = static_cast<latte::SQ_TEX_CLAMP>(clampY);
   sampler->regs.word0.CLAMP_Z = static_cast<latte::SQ_TEX_CLAMP>(clampZ);
}

void
GX2InitSamplerDepthCompare(GX2Sampler *sampler,
                           GX2CompareFunction::Value depthCompare)
{
   sampler->regs.word0.DEPTH_COMPARE_FUNCTION = static_cast<latte::SQ_TEX_DEPTH_COMPARE>(depthCompare);
}

void
GX2InitSamplerFilterAdjust(GX2Sampler *sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode::Value perfMip,
                           GX2TexZPerfMode::Value perfZ)
{
   sampler->regs.word2.HIGH_PRECISION_FILTER = highPrecision;
   sampler->regs.word2.PERF_MIP = perfMip;
   sampler->regs.word2.PERF_Z = perfZ;
}

void
GX2InitSamplerLOD(GX2Sampler *sampler,
                  float lodMin,
                  float lodMax,
                  float lodBias)
{
   sampler->regs.word1.MIN_LOD = floatToFixedPoint(lodMin, 10, 0.0f, 16.0f);
   sampler->regs.word1.MAX_LOD = floatToFixedPoint(lodMax, 10, 0.0f, 16.0f);
   sampler->regs.word1.LOD_BIAS = floatToFixedPoint(lodBias, 12, -32.0f, 32.0f);
}

void
GX2InitSamplerLODAdjust(GX2Sampler *sampler,
                        float anisoBias,
                        BOOL lodUsesMinorAxis)
{
   sampler->regs.word2.ANISO_BIAS = floatToFixedPoint(anisoBias, 6, 0.0f, 2.0f);
   sampler->regs.word0.LOD_USES_MINOR_AXIS = lodUsesMinorAxis;
}

void
GX2InitSamplerRoundingMode(GX2Sampler *sampler,
                           GX2RoundingMode::Value roundingMode)
{
   sampler->regs.word2.TRUNCATE_COORD = roundingMode;
}

void
GX2InitSamplerXYFilter(GX2Sampler *sampler,
                       GX2TexXYFilterMode::Value filterMag,
                       GX2TexXYFilterMode::Value filterMin,
                       GX2TexAnisoRatio::Value maxAniso)
{
   sampler->regs.word0.XY_MAG_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(filterMag);
   sampler->regs.word0.XY_MIN_FILTER = static_cast<latte::SQ_TEX_XY_FILTER>(filterMin);
   sampler->regs.word0.MAX_ANISO_RATIO = static_cast<latte::SQ_TEX_ANISO>(maxAniso);
}

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
                       GX2TexZFilterMode::Value filterZ,
                       GX2TexMipFilterMode::Value filterMip)
{
   sampler->regs.word0.Z_FILTER = static_cast<latte::SQ_TEX_Z_FILTER>(filterZ);
   sampler->regs.word0.MIP_FILTER = static_cast<latte::SQ_TEX_Z_FILTER>(filterMip);
}
