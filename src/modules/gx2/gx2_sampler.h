#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

struct GX2Sampler
{
   struct
   {
      GX2TexClampMode::Value clampX : 3;
      GX2TexClampMode::Value clampY : 3;
      GX2TexClampMode::Value clampZ : 3;
      GX2TexXYFilterMode::Value magFilter : 3;
      GX2TexXYFilterMode::Value minFilter : 3;
      GX2TexZFilterMode::Value filterZ : 2;
      GX2TexMipFilterMode::Value filterMip : 2;
      uint32_t unkFilterXY : 3; // GX2InitSamplerXYFilter unk
      GX2TexBorderType::Value borderType : 2;
      uint32_t : 2;
      GX2CompareFunction::Value depthCompare : 3;
      uint32_t : 2;
      uint32_t unkLodAdjust2 : 1; // GX2InitSamplerLODAdjust unk2
   };

   struct
   {
      uint32_t lodMin : 10; // float 0 ... 16
      uint32_t lodMax : 10; // float 0 ... 16
      uint32_t lodBias : 12; // float -32 ... 32
   };

   struct {
      uint32_t : 15;
      uint32_t unkFilterAdjust : 1; // GX2InitSamplerFilterAdjust unk
      uint32_t perfMip : 3;
      uint32_t perfZ : 2;
      uint32_t unkLodAdjust1 : 6; // GX2InitSamplerLODAdjust unk1 float 0 ... 2
      uint32_t : 2;
      GX2RoundingMode::Value roundingMode : 1;
      uint32_t : 2;
   };
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
                           BOOL unk,
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
                       GX2TexXYFilterMode::Value filterX,
                       GX2TexXYFilterMode::Value filterY,
                       uint32_t unk);

void
GX2InitSamplerZMFilter(GX2Sampler *sampler,
                       GX2TexZFilterMode::Value filterZ,
                       GX2TexMipFilterMode::Value filterMip);