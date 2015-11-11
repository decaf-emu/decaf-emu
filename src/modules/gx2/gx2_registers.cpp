#include "gpu/pm4.h"
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
   reg->colorSrcBlend = colorSrcBlend;
   reg->colorDstBlend = colorDstBlend;
   reg->colorCombine = colorCombine;
   reg->useAlphaBlend = useAlphaBlend;
   reg->alphaSrcBlend = alphaSrcBlend;
   reg->alphaDstBlend = alphaDstBlend;
   reg->alphaCombine = alphaCombine;
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
   auto id = static_cast<pm4::ContextRegister::Value>(pm4::ContextRegister::Blend0Control + reg->target);
   pm4::setContextRegister(id, reg->dw1);
}

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
                              GX2StencilFunction::Value backStencilFail)
{
   reg->depthTest = depthTest;
   reg->depthWrite = depthWrite;
   reg->depthCompare = depthCompare;
   reg->stencilTest = stencilTest;
   reg->unk1 = unk1;
   reg->frontStencilFunc = frontStencilFunc;
   reg->frontStencilZPass = frontStencilZPass;
   reg->frontStencilZFail = frontStencilZFail;
   reg->frontStencilFail = frontStencilFail;
   reg->backStencilFunc = backStencilFunc;
   reg->backStencilZPass = backStencilZPass;
   reg->backStencilZFail = backStencilZFail;
   reg->backStencilFail = backStencilFail;
}

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
                          GX2StencilFunction::Value backStencilFail)
{
   GX2DepthStencilControlReg reg;
   GX2InitDepthStencilControlReg(&reg,
                                 depthTest,
                                 depthWrite,
                                 depthCompare,
                                 stencilTest,
                                 unk1,
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
   // TODO: GX2SetDepthStencilControlReg
}