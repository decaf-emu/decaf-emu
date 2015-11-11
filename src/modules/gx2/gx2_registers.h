#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

struct GX2BlendControlReg
{
   GX2RenderTarget::Value target;

   union
   {
      uint32_t dw1;

      struct
      {
         GX2BlendMode::Value colorSrcBlend : 5;
         GX2BlendCombineMode::Value colorCombine : 3;
         GX2BlendMode::Value colorDstBlend : 5;
         uint32_t : 3; // padding
         GX2BlendMode::Value alphaSrcBlend : 5;
         GX2BlendCombineMode::Value alphaCombine : 3;
         GX2BlendMode::Value alphaDstBlend : 5;
         uint32_t useAlphaBlend : 1;
         uint32_t : 2; // padding
      };
   };
};
CHECK_SIZE(GX2BlendControlReg, 8);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x00 + 32, colorSrcBlend);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x05 + 32, colorCombine);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x08 + 32, colorDstBlend);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x10 + 32, alphaSrcBlend);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x15 + 32, alphaCombine);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x18 + 32, alphaDstBlend);
CHECK_BIT_OFFSET(GX2BlendControlReg, 0x1D + 32, useAlphaBlend);

struct GX2DepthStencilControlReg
{
   uint32_t stencilTest : 1;
   uint32_t depthTest : 1;
   uint32_t depthWrite : 1;
   uint32_t : 1; // padding
   GX2CompareFunction::Value depthCompare : 3;
   uint32_t unk1 : 1; // unk1 from GX2InitDepthStencilControlReg
   GX2CompareFunction::Value frontStencilFunc : 3;
   GX2StencilFunction::Value frontStencilFail : 3;
   GX2StencilFunction::Value frontStencilZPass : 3;
   GX2StencilFunction::Value frontStencilZFail : 3;
   GX2CompareFunction::Value backStencilFunc : 3;
   GX2StencilFunction::Value backStencilFail : 3;
   GX2StencilFunction::Value backStencilZPass : 3;
   GX2StencilFunction::Value backStencilZFail : 3;
};
CHECK_SIZE(GX2DepthStencilControlReg, 4);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x00, stencilTest);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x01, depthTest);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x02, depthWrite);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x04, depthCompare);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x08, frontStencilFunc);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x0B, frontStencilFail);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x0E, frontStencilZPass);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x11, frontStencilZFail);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x14, backStencilFunc);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x17, backStencilFail);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x1A, backStencilZPass);
CHECK_BIT_OFFSET(GX2DepthStencilControlReg, 0x1D, backStencilZFail);

#pragma pack(pop)

void
GX2InitBlendControlReg(GX2BlendControlReg *reg,
                       GX2RenderTarget::Value target,
                       GX2BlendMode::Value colorSrcBlend,
                       GX2BlendMode::Value colorDstBlend,
                       GX2BlendCombineMode::Value colorCombine,
                       BOOL useAlphaBlend,
                       GX2BlendMode::Value alphaSrcBlend,
                       GX2BlendMode::Value alphaDstBlend,
                       GX2BlendCombineMode::Value alphaCombine);

void
GX2SetBlendControl(GX2RenderTarget::Value target,
                   GX2BlendMode::Value colorSrcBlend,
                   GX2BlendMode::Value colorDstBlend,
                   GX2BlendCombineMode::Value colorCombine,
                   BOOL useAlphaBlend,
                   GX2BlendMode::Value alphaSrcBlend,
                   GX2BlendMode::Value alphaDstBlend,
                   GX2BlendCombineMode::Value alphaCombine);

void
GX2SetBlendControlReg(GX2BlendControlReg *reg);

void
GX2InitDepthStencilControlReg(GX2DepthStencilControlReg *reg,
                              BOOL depthTest,
                              BOOL depthWrite,
                              GX2CompareFunction::Value depthCompare,
                              BOOL stencilTest,
                              BOOL unk1,
                              GX2CompareFunction::Value frontStencilFunc,
                              GX2StencilFunction::Value frontStencilZPass,
                              GX2StencilFunction::Value frontStencilZFail,
                              GX2StencilFunction::Value frontStencilFail,
                              GX2CompareFunction::Value backStencilFunc,
                              GX2StencilFunction::Value backStencilZPass,
                              GX2StencilFunction::Value backStencilZFail,
                              GX2StencilFunction::Value backStencilFail);

void
GX2SetDepthStencilControl(BOOL depthTest,
                          BOOL depthWrite,
                          GX2CompareFunction::Value depthCompare,
                          BOOL stencilTest,
                          BOOL unk1,
                          GX2CompareFunction::Value frontStencilFunc,
                          GX2StencilFunction::Value frontStencilZPass,
                          GX2StencilFunction::Value frontStencilZFail,
                          GX2StencilFunction::Value frontStencilFail,
                          GX2CompareFunction::Value backStencilFunc,
                          GX2StencilFunction::Value backStencilZPass,
                          GX2StencilFunction::Value backStencilZFail,
                          GX2StencilFunction::Value backStencilFail);

void
GX2SetDepthStencilControlReg(GX2DepthStencilControlReg *reg);
