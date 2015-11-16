#include "gpu/pm4.h"
#include "gpu/pm4_writer.h"
#include "gx2_registers.h"


void
GX2InitBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                             float red,
                             float green,
                             float blue,
                             float alpha)
{
   reg->red = red;
   reg->green = green;
   reg->blue = blue;
   reg->alpha = alpha;
}


void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha)
{
   GX2BlendConstantColorReg reg;
   GX2InitBlendConstantColorReg(&reg, red, green, blue, alpha);
   GX2SetBlendConstantColorReg(&reg);
}


void
GX2SetBlendConstantColorReg(GX2BlendConstantColorReg *reg)
{
   float colors[] = { reg->red, reg->green, reg->blue, reg->alpha };
   auto values = reinterpret_cast<uint32_t*>(colors);
   pm4::write(pm4::SetContextRegs { latte::Register::CB_BLEND_RED, { values, 4 } });
}


void
GX2GetBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                            float *red,
                            float *green,
                            float *blue,
                            float *alpha)
{
   *red = reg->red;
   *green = reg->green;
   *blue = reg->blue;
   *alpha = reg->alpha;
}


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
   auto cb_blend_control = reg->cb_blend_control.value();
   reg->target = target;
   cb_blend_control.COLOR_SRCBLEND        = static_cast<latte::CB_BLEND_FUNC>(colorSrcBlend);
   cb_blend_control.COLOR_DESTBLEND       = static_cast<latte::CB_BLEND_FUNC>(colorDstBlend);
   cb_blend_control.COLOR_COMB_FCN        = static_cast<latte::CB_COMB_FUNC>(colorCombine);
   cb_blend_control.SEPARATE_ALPHA_BLEND  = useAlphaBlend;
   cb_blend_control.ALPHA_SRCBLEND        = static_cast<latte::CB_BLEND_FUNC>(alphaSrcBlend);
   cb_blend_control.ALPHA_DESTBLEND       = static_cast<latte::CB_BLEND_FUNC>(alphaDstBlend);
   cb_blend_control.ALPHA_COMB_FCN        = static_cast<latte::CB_COMB_FUNC>(alphaCombine);
   reg->cb_blend_control = cb_blend_control;
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
   auto cb_blend_control = reg->cb_blend_control.value();
   auto id = static_cast<latte::Register::Value>(latte::Register::CB_BLEND0_CONTROL + reg->target);
   pm4::write(pm4::SetContextReg { id, cb_blend_control.value });
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
   auto db_depth_control = reg->db_depth_control.value();
   db_depth_control.Z_ENABLE         = depthTest;
   db_depth_control.Z_WRITE_ENABLE   = depthWrite;
   db_depth_control.ZFUNC            = static_cast<latte::DB_FRAG_FUNC>(depthCompare);
   db_depth_control.STENCIL_ENABLE   = stencilTest;
   db_depth_control.BACKFACE_ENABLE  = backfaceStencil;
   db_depth_control.STENCILFUNC      = static_cast<latte::DB_REF_FUNC>(frontStencilFunc);
   db_depth_control.STENCILZPASS     = static_cast<latte::DB_STENCIL_FUNC>(frontStencilZPass);
   db_depth_control.STENCILZFAIL     = static_cast<latte::DB_STENCIL_FUNC>(frontStencilZFail);
   db_depth_control.STENCILFAIL      = static_cast<latte::DB_STENCIL_FUNC>(frontStencilFail);
   db_depth_control.STENCILFUNC_BF   = static_cast<latte::DB_REF_FUNC>(backStencilFunc);
   db_depth_control.STENCILZPASS_BF  = static_cast<latte::DB_STENCIL_FUNC>(backStencilZPass);
   db_depth_control.STENCILZFAIL_BF  = static_cast<latte::DB_STENCIL_FUNC>(backStencilZFail);
   db_depth_control.STENCILFAIL_BF   = static_cast<latte::DB_STENCIL_FUNC>(backStencilFail);
   reg->db_depth_control = db_depth_control;
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
   auto db_depth_control = reg->db_depth_control.value();
   pm4::write(pm4::SetContextReg { latte::Register::DB_DEPTH_CONTROL, db_depth_control.value });
}

void
GX2SetPrimitiveRestartIndex(uint32_t index)
{
   pm4::write(pm4::SetContextReg { latte::Register::VGT_MULTI_PRIM_IB_RESET_INDX, index });
}
