#include "gpu/pm4.h"
#include "gpu/pm4_writer.h"
#include "gx2_registers.h"

void
GX2InitBlendControlReg(GX2BlendControlReg *reg,
                       GX2RenderTarget::Value target,
                       GX2BlendMode::Value colorSrcBlend,
                       GX2BlendMode::Value colorDstBlend,
                       GX2BlendCombineMode::Value colorCombine,
                       BOOL useAlphaBlend,
                       GX2BlendMode::Value alphaSrcBlend,
                       GX2BlendMode::Value alphaDstBlend,
                       GX2BlendCombineMode::Value alphaCombine)
{
   reg->target = target;
   reg->value.colorSrcBlend = colorSrcBlend;
   reg->value.colorDstBlend = colorDstBlend;
   reg->value.colorCombine = colorCombine;
   reg->value.useAlphaBlend = useAlphaBlend;
   reg->value.alphaSrcBlend = alphaSrcBlend;
   reg->value.alphaDstBlend = alphaDstBlend;
   reg->value.alphaCombine = alphaCombine;
}

void
GX2SetBlendControl(GX2RenderTarget::Value target,
                   GX2BlendMode::Value colorSrcBlend,
                   GX2BlendMode::Value colorDstBlend,
                   GX2BlendCombineMode::Value colorCombine,
                   BOOL useAlphaBlend,
                   GX2BlendMode::Value alphaSrcBlend,
                   GX2BlendMode::Value alphaDstBlend,
                   GX2BlendCombineMode::Value alphaCombine)
{
   GX2BlendControlReg reg;
   GX2InitBlendControlReg(&reg,
                          target,
                          colorSrcBlend,
                          colorDstBlend,
                          colorCombine,
                          useAlphaBlend,
                          alphaSrcBlend,
                          alphaDstBlend,
                          alphaCombine);
   GX2SetBlendControlReg(&reg);
}

void
GX2SetBlendControlReg(GX2BlendControlReg *reg)
{
   auto id = static_cast<latte::Register::Value>(latte::Register::Blend0Control + reg->target);
   pm4::write(pm4::SetContextReg { id, { &reg->value.value, 1 } });
}

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
                              GX2StencilFunction::Value backStencilFail)
{
   reg->value.depthTest = depthTest;
   reg->value.depthWrite = depthWrite;
   reg->value.depthCompare = depthCompare;
   reg->value.stencilTest = stencilTest;
   reg->value.backfaceStencil = backfaceStencil;
   reg->value.frontStencilFunc = frontStencilFunc;
   reg->value.frontStencilZPass = frontStencilZPass;
   reg->value.frontStencilZFail = frontStencilZFail;
   reg->value.frontStencilFail = frontStencilFail;
   reg->value.backStencilFunc = backStencilFunc;
   reg->value.backStencilZPass = backStencilZPass;
   reg->value.backStencilZFail = backStencilZFail;
   reg->value.backStencilFail = backStencilFail;
}

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
                          GX2StencilFunction::Value backStencilFail)
{
   GX2DepthStencilControlReg reg;
   GX2InitDepthStencilControlReg(&reg,
                                 depthTest,
                                 depthWrite,
                                 depthCompare,
                                 stencilTest,
                                 backfaceStencil,
                                 frontStencilFunc,
                                 frontStencilZPass,
                                 frontStencilZFail,
                                 frontStencilFail,
                                 backStencilFunc,
                                 backStencilZPass,
                                 backStencilZFail,
                                 backStencilFail);
   GX2SetDepthStencilControlReg(&reg);
}

void
GX2SetDepthStencilControlReg(GX2DepthStencilControlReg *reg)
{
   pm4::write(pm4::SetContextReg { latte::Register::DepthControl, { &reg->value.value, 1 } });
}