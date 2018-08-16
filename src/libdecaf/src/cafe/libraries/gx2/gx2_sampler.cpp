#include "gx2.h"
#include "gx2_internal_cbpool.h"
#include "gx2_sampler.h"

#include <algorithm>

namespace cafe::gx2
{

inline uint32_t
floatToFixedPoint(float value,
                  uint32_t bits,
                  float min,
                  float max)
{
   return static_cast<uint32_t>((value - min) * (static_cast<float>(1 << bits) / (max - min)));
}

void
GX2InitSampler(virt_ptr<GX2Sampler> sampler,
               GX2TexClampMode clampMode,
               GX2TexXYFilterMode minMagFilterMode)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .CLAMP_X(static_cast<latte::SQ_TEX_CLAMP>(clampMode))
      .CLAMP_Y(static_cast<latte::SQ_TEX_CLAMP>(clampMode))
      .CLAMP_Z(static_cast<latte::SQ_TEX_CLAMP>(clampMode))
      .XY_MAG_FILTER(static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode))
      .XY_MIN_FILTER(static_cast<latte::SQ_TEX_XY_FILTER>(minMagFilterMode));

   sampler->regs.word0 = word0;
}

void
GX2InitSamplerBorderType(virt_ptr<GX2Sampler> sampler,
                         GX2TexBorderType borderType)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .BORDER_COLOR_TYPE(static_cast<latte::SQ_TEX_BORDER_COLOR>(borderType));

   sampler->regs.word0 = word0;
}

void
GX2InitSamplerClamping(virt_ptr<GX2Sampler> sampler,
                       GX2TexClampMode clampX,
                       GX2TexClampMode clampY,
                       GX2TexClampMode clampZ)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .CLAMP_X(static_cast<latte::SQ_TEX_CLAMP>(clampX))
      .CLAMP_Y(static_cast<latte::SQ_TEX_CLAMP>(clampY))
      .CLAMP_Z(static_cast<latte::SQ_TEX_CLAMP>(clampZ));

   sampler->regs.word0 = word0;
}

void
GX2InitSamplerDepthCompare(virt_ptr<GX2Sampler> sampler,
                           GX2CompareFunction depthCompare)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .DEPTH_COMPARE_FUNCTION(static_cast<latte::REF_FUNC>(depthCompare));

   sampler->regs.word0 = word0;
}

void
GX2InitSamplerFilterAdjust(virt_ptr<GX2Sampler> sampler,
                           BOOL highPrecision,
                           GX2TexMipPerfMode perfMip,
                           GX2TexZPerfMode perfZ)
{
   auto word2 = sampler->regs.word2.value();

   word2 = word2
      .HIGH_PRECISION_FILTER(!!highPrecision)
      .PERF_MIP(perfMip)
      .PERF_Z(perfZ);

   sampler->regs.word2 = word2;
}

void
GX2InitSamplerLOD(virt_ptr<GX2Sampler> sampler,
                  float lodMin,
                  float lodMax,
                  float lodBias)
{
   auto word1 = sampler->regs.word1.value();

   lodMin = std::min(std::max(lodMin, 0.0f), 16.0f);
   lodMax = std::min(std::max(lodMax, 0.0f), 16.0f);
   lodBias = std::min(std::max(lodBias, -32.0f), 32.0f);

   word1 = word1
      .MIN_LOD(ufixed_4_6_t { lodMin })
      .MAX_LOD(ufixed_4_6_t { lodMax })
      .LOD_BIAS(sfixed_1_5_6_t { lodBias });

   sampler->regs.word1 = word1;
}

void
GX2InitSamplerLODAdjust(virt_ptr<GX2Sampler> sampler,
                        float anisoBias,
                        BOOL lodUsesMinorAxis)
{
   auto word0 = sampler->regs.word0.value();
   auto word2 = sampler->regs.word2.value();

   anisoBias = std::min(std::max(anisoBias, 0.0f), 2.0f);

   word2 = word2
      .ANISO_BIAS(ufixed_1_5_t { anisoBias });

   word0 = word0
      .LOD_USES_MINOR_AXIS(!!lodUsesMinorAxis);

   sampler->regs.word0 = word0;
   sampler->regs.word2 = word2;
}

void
GX2InitSamplerRoundingMode(virt_ptr<GX2Sampler> sampler,
                           GX2RoundingMode roundingMode)
{
   auto word2 = sampler->regs.word2.value();

   word2 = word2
      .TRUNCATE_COORD(static_cast<latte::SQ_TEX_ROUNDING_MODE>(roundingMode));

   sampler->regs.word2 = word2;
}

void
GX2InitSamplerXYFilter(virt_ptr<GX2Sampler> sampler,
                       GX2TexXYFilterMode filterMag,
                       GX2TexXYFilterMode filterMin,
                       GX2TexAnisoRatio maxAniso)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .XY_MAG_FILTER(static_cast<latte::SQ_TEX_XY_FILTER>(filterMag))
      .XY_MIN_FILTER(static_cast<latte::SQ_TEX_XY_FILTER>(filterMin))
      .MAX_ANISO_RATIO(static_cast<latte::SQ_TEX_ANISO>(maxAniso));

   sampler->regs.word0 = word0;
}

void
GX2InitSamplerZMFilter(virt_ptr<GX2Sampler> sampler,
                       GX2TexZFilterMode filterZ,
                       GX2TexMipFilterMode filterMip)
{
   auto word0 = sampler->regs.word0.value();

   word0 = word0
      .Z_FILTER(static_cast<latte::SQ_TEX_Z_FILTER>(filterZ))
      .MIP_FILTER(static_cast<latte::SQ_TEX_Z_FILTER>(filterMip));

   sampler->regs.word0 = word0;
}

void
GX2SetPixelSamplerBorderColor(uint32_t unit,
                              float red,
                              float green,
                              float blue,
                              float alpha)
{
   uint32_t values[] = {
      bit_cast<uint32_t>(red),
      bit_cast<uint32_t>(green),
      bit_cast<uint32_t>(blue),
      bit_cast<uint32_t>(alpha),
   };

   auto id = latte::Register::TD_PS_SAMPLER_BORDER0_RED + 4 * (unit * 4);
   internal::writePM4(latte::pm4::SetConfigRegs {
      static_cast<latte::Register>(id),
      gsl::make_span(values)
   });
}


void
GX2SetVertexSamplerBorderColor(uint32_t unit,
                                 float red,
                                 float green,
                                 float blue,
                                 float alpha)
{
   uint32_t values[] = {
      bit_cast<uint32_t>(red),
      bit_cast<uint32_t>(green),
      bit_cast<uint32_t>(blue),
      bit_cast<uint32_t>(alpha),
   };

   auto id = latte::Register::TD_VS_SAMPLER_BORDER0_RED + 4 * (unit * 4);
   internal::writePM4(latte::pm4::SetConfigRegs {
      static_cast<latte::Register>(id),
      gsl::make_span(values)
   });
}

void
GX2SetGeometrySamplerBorderColor(uint32_t unit,
                                 float red,
                                 float green,
                                 float blue,
                                 float alpha)
{
   uint32_t values[] = {
      bit_cast<uint32_t>(red),
      bit_cast<uint32_t>(green),
      bit_cast<uint32_t>(blue),
      bit_cast<uint32_t>(alpha),
   };

   auto id = latte::Register::TD_GS_SAMPLER_BORDER0_RED + 4 * (unit * 4);
   internal::writePM4(latte::pm4::SetConfigRegs {
      static_cast<latte::Register>(id),
      gsl::make_span(values)
   });
}

void
Library::registerSamplerSymbols()
{
   RegisterFunctionExport(GX2InitSampler);
   RegisterFunctionExport(GX2InitSamplerBorderType);
   RegisterFunctionExport(GX2InitSamplerClamping);
   RegisterFunctionExport(GX2InitSamplerDepthCompare);
   RegisterFunctionExport(GX2InitSamplerFilterAdjust);
   RegisterFunctionExport(GX2InitSamplerLOD);
   RegisterFunctionExport(GX2InitSamplerLODAdjust);
   RegisterFunctionExport(GX2InitSamplerRoundingMode);
   RegisterFunctionExport(GX2InitSamplerXYFilter);
   RegisterFunctionExport(GX2InitSamplerZMFilter);
   RegisterFunctionExport(GX2SetPixelSamplerBorderColor);
   RegisterFunctionExport(GX2SetVertexSamplerBorderColor);
   RegisterFunctionExport(GX2SetGeometrySamplerBorderColor);
}

} // namespace cafe::gx2
