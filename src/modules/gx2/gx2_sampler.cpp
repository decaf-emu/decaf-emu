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
   sampler->clampX = clampMode;
   sampler->clampY = clampMode;
   sampler->clampZ = clampMode;
   sampler->magFilter = minMagFilterMode;
   sampler->minFilter = minMagFilterMode;
}

void
GX2InitSamplerBorderType(GX2Sampler *sampler,
                         GX2TexBorderType::Value borderType)
{
   sampler->borderType = borderType;
}

void
GX2InitSamplerClamping(GX2Sampler *sampler,
                       GX2TexClampMode::Value clampX,
                       GX2TexClampMode::Value clampY,
                       GX2TexClampMode::Value clampZ)
{
   sampler->clampX = clampX;
   sampler->clampY = clampY;
   sampler->clampZ = clampZ;
}

void
GX2InitSamplerDepthCompare(GX2Sampler *sampler,
                           GX2CompareFunction::Value depthCompare)
{
   sampler->depthCompare = depthCompare;
}

void
GX2InitSamplerFilterAdjust(GX2Sampler *sampler,
                           BOOL unk,
                           GX2TexMipPerfMode::Value perfMip,
                           GX2TexZPerfMode::Value perfZ)
{
   sampler->unkFilterAdjust = unk;
   sampler->perfMip = perfMip;
   sampler->perfZ = perfZ;
}

void
GX2InitSamplerLOD(GX2Sampler *sampler,
                  float lodMin,
                  float lodMax,
                  float lodBias)
{
   sampler->lodMin = floatToFixedPoint(lodMin, 10, 0.0f, 16.0f);
   sampler->lodMax = floatToFixedPoint(lodMax, 10, 0.0f, 16.0f);
   sampler->lodBias = floatToFixedPoint(lodBias, 12, -32.0f, 32.0f);
}

void
GX2InitSamplerLODAdjust(GX2Sampler *sampler,
                        float unk1,
                        BOOL unk2)
{
   sampler->unkLodAdjust1 = floatToFixedPoint(unk1, 6, 0.0f, 2.0f);
   sampler->unkLodAdjust2 = unk2;
}

void
GX2InitSamplerRoundingMode(GX2Sampler *sampler,
                           GX2RoundingMode::Value roundingMode)
{
   sampler->roundingMode = roundingMode;
}

void
GX2InitSamplerXYFilter(GX2Sampler *sampler,
                       GX2TexXYFilterMode::Value filterX,
                       GX2TexXYFilterMode::Value filterY,
                       uint32_t unk)
{
   sampler->magFilter = filterX;
   sampler->minFilter = filterX;
   sampler->unkFilterXY = unk;
}

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
                       GX2TexZFilterMode::Value filterZ,
                       GX2TexMipFilterMode::Value filterMip)
{
   sampler->filterZ = filterZ;
   sampler->filterMip = filterMip;
}
