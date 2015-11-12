#pragma once
#include "types.h"
#include "gpu/latte_registers.h"
#include "gx2_enum.h"
#include "utils/structsize.h"

#pragma pack(push, 1)

struct GX2BlendControlReg
{
   GX2RenderTarget::Value target;
   latte::CB_BLEND_CONTROL value;
};
CHECK_SIZE(GX2BlendControlReg, 8);
CHECK_OFFSET(GX2BlendControlReg, 0, target);
CHECK_OFFSET(GX2BlendControlReg, 4, value);

struct GX2BlendConstantColorReg
{
   float red;
   float green;
   float blue;
   float alpha;
};
CHECK_SIZE(GX2BlendConstantColorReg, 0x10);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x00, red);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x04, green);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x08, blue);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x0c, alpha);

struct GX2DepthStencilControlReg
{
   latte::DB_DEPTH_CONTROL value;
};
CHECK_SIZE(GX2DepthStencilControlReg, 4);
CHECK_OFFSET(GX2DepthStencilControlReg, 0, value);

#pragma pack(pop)

void
GX2InitBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                             float red,
                             float green,
                             float blue,
                             float alpha);

void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha);

void
GX2SetBlendConstantColorReg(GX2BlendConstantColorReg *reg);

void
GX2GetBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                            float *red,
                            float *green,
                            float *blue,
                            float *alpha);

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
                              BOOL backfaceStencil,
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
                          BOOL backfaceStencil,
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
