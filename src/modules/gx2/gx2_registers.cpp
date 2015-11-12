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
   reg->value.colorSrcBlend = static_cast<latte::BLEND_FUNC>(colorSrcBlend);
   reg->value.colorDstBlend = static_cast<latte::BLEND_FUNC>(colorDstBlend);
   reg->value.colorCombine = static_cast<latte::COMB_FUNC>(colorCombine);
   reg->value.useAlphaBlend = useAlphaBlend;
   reg->value.alphaSrcBlend = static_cast<latte::BLEND_FUNC>(alphaSrcBlend);
   reg->value.alphaDstBlend = static_cast<latte::BLEND_FUNC>(alphaDstBlend);
   reg->value.alphaCombine = static_cast<latte::COMB_FUNC>(alphaCombine);
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
   reg->value.depthCompare = static_cast<latte::FRAG_FUNC>(depthCompare);
   reg->value.stencilTest = stencilTest;
   reg->value.backfaceStencil = backfaceStencil;
   reg->value.frontStencilFunc = static_cast<latte::REF_FUNC>(frontStencilFunc);
   reg->value.frontStencilZPass = static_cast<latte::STENCIL_FUNC>(frontStencilZPass);
   reg->value.frontStencilZFail = static_cast<latte::STENCIL_FUNC>(frontStencilZFail);
   reg->value.frontStencilFail = static_cast<latte::STENCIL_FUNC>(frontStencilFail);
   reg->value.backStencilFunc = static_cast<latte::REF_FUNC>(backStencilFunc);
   reg->value.backStencilZPass = static_cast<latte::STENCIL_FUNC>(backStencilZPass);
   reg->value.backStencilZFail = static_cast<latte::STENCIL_FUNC>(backStencilZFail);
   reg->value.backStencilFail = static_cast<latte::STENCIL_FUNC>(backStencilFail);
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