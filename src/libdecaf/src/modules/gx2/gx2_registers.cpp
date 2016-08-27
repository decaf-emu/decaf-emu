#include "gpu/pm4_packets.h"
#include "gpu/pm4_writer.h"
#include "gx2_registers.h"

namespace gx2
{

void
GX2SetAAMask(uint8_t upperLeft,
             uint8_t upperRight,
             uint8_t lowerLeft,
             uint8_t lowerRight)
{
   GX2AAMaskReg reg;
   GX2InitAAMaskReg(&reg,
                    upperLeft,
                    upperRight,
                    lowerLeft,
                    lowerRight);
   GX2SetAAMaskReg(&reg);
}

void
GX2InitAAMaskReg(GX2AAMaskReg *reg,
                 uint8_t upperLeft,
                 uint8_t upperRight,
                 uint8_t lowerLeft,
                 uint8_t lowerRight)
{
   auto pa_sc_aa_mask = reg->pa_sc_aa_mask.value();

   pa_sc_aa_mask = pa_sc_aa_mask
      .AA_MASK_ULC(upperLeft)
      .AA_MASK_URC(upperRight)
      .AA_MASK_LLC(lowerLeft)
      .AA_MASK_LRC(lowerRight);

   reg->pa_sc_aa_mask = pa_sc_aa_mask;
}

void
GX2GetAAMaskReg(GX2AAMaskReg *reg,
                be_val<uint8_t> *upperLeft,
                be_val<uint8_t> *upperRight,
                be_val<uint8_t> *lowerLeft,
                be_val<uint8_t> *lowerRight)
{
   auto pa_sc_aa_mask = reg->pa_sc_aa_mask.value();
   *upperLeft = pa_sc_aa_mask.AA_MASK_ULC();
   *upperRight = pa_sc_aa_mask.AA_MASK_URC();
   *lowerLeft = pa_sc_aa_mask.AA_MASK_LLC();
   *lowerRight = pa_sc_aa_mask.AA_MASK_LRC();
}

void
GX2SetAAMaskReg(GX2AAMaskReg *reg)
{
   auto pa_sc_aa_mask = reg->pa_sc_aa_mask.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SC_AA_MASK, pa_sc_aa_mask.value });
}

void
GX2SetAlphaTest(BOOL alphaTest,
                GX2CompareFunction func,
                float ref)
{
   GX2AlphaTestReg reg;
   GX2InitAlphaTestReg(&reg, alphaTest, func, ref);
   GX2SetAlphaTestReg(&reg);
}

void
GX2InitAlphaTestReg(GX2AlphaTestReg *reg,
                    BOOL alphaTest,
                    GX2CompareFunction func,
                    float ref)
{
   auto sx_alpha_ref = reg->sx_alpha_ref.value();
   auto sx_alpha_test_control = reg->sx_alpha_test_control.value();

   sx_alpha_test_control = sx_alpha_test_control
      .ALPHA_TEST_ENABLE(!!alphaTest)
      .ALPHA_FUNC(static_cast<latte::REF_FUNC>(func));

   sx_alpha_ref.ALPHA_REF = ref;

   reg->sx_alpha_ref = sx_alpha_ref;
   reg->sx_alpha_test_control = sx_alpha_test_control;
}

void
GX2GetAlphaTestReg(const GX2AlphaTestReg *reg,
                   be_val<BOOL> *alphaTest,
                   be_val<GX2CompareFunction> *func,
                   be_val<float> *ref)
{
   auto sx_alpha_ref = reg->sx_alpha_ref.value();
   auto sx_alpha_test_control = reg->sx_alpha_test_control.value();

   *alphaTest = sx_alpha_test_control.ALPHA_TEST_ENABLE();
   *func = static_cast<GX2CompareFunction>(sx_alpha_test_control.ALPHA_FUNC());
   *ref = sx_alpha_ref.ALPHA_REF;
}

void
GX2SetAlphaTestReg(GX2AlphaTestReg *reg)
{
   auto sx_alpha_test_control = reg->sx_alpha_test_control.value();
   pm4::write(pm4::SetContextReg { latte::Register::SX_ALPHA_TEST_CONTROL, sx_alpha_test_control.value });

   auto sx_alpha_ref = reg->sx_alpha_ref.value();
   pm4::write(pm4::SetContextReg { latte::Register::SX_ALPHA_REF, sx_alpha_ref.value });
}

void
GX2SetAlphaToMask(BOOL alphaToMask,
                  GX2AlphaToMaskMode mode)
{
   GX2AlphaToMaskReg reg;
   GX2InitAlphaToMaskReg(&reg, alphaToMask, mode);
   GX2SetAlphaToMaskReg(&reg);
}

void
GX2InitAlphaToMaskReg(GX2AlphaToMaskReg *reg,
                      BOOL alphaToMask,
                      GX2AlphaToMaskMode mode)
{
   auto db_alpha_to_mask = reg->db_alpha_to_mask.value();
   db_alpha_to_mask = db_alpha_to_mask
      .ALPHA_TO_MASK_ENABLE(!!alphaToMask);

   switch (mode) {
   case GX2AlphaToMaskMode::NonDithered:
      // 0xAA = 10 10 10 10
      db_alpha_to_mask = db_alpha_to_mask
         .ALPHA_TO_MASK_OFFSET0(2)
         .ALPHA_TO_MASK_OFFSET1(2)
         .ALPHA_TO_MASK_OFFSET2(2)
         .ALPHA_TO_MASK_OFFSET3(2);
      break;
   case GX2AlphaToMaskMode::Dither0:
      // 0x78 = 01 11 10 00
      db_alpha_to_mask = db_alpha_to_mask
         .ALPHA_TO_MASK_OFFSET0(0)
         .ALPHA_TO_MASK_OFFSET1(2)
         .ALPHA_TO_MASK_OFFSET2(3)
         .ALPHA_TO_MASK_OFFSET3(1);
      break;
   case GX2AlphaToMaskMode::Dither90:
      // 0xC6 = 11 00 01 10
      db_alpha_to_mask = db_alpha_to_mask
         .ALPHA_TO_MASK_OFFSET0(2)
         .ALPHA_TO_MASK_OFFSET1(1)
         .ALPHA_TO_MASK_OFFSET2(0)
         .ALPHA_TO_MASK_OFFSET3(3);
      break;
   case GX2AlphaToMaskMode::Dither180:
      // 0x2D = 00 10 11 01
      db_alpha_to_mask = db_alpha_to_mask
         .ALPHA_TO_MASK_OFFSET0(1)
         .ALPHA_TO_MASK_OFFSET1(3)
         .ALPHA_TO_MASK_OFFSET2(2)
         .ALPHA_TO_MASK_OFFSET3(0);
      break;
   case GX2AlphaToMaskMode::Dither270:
      // 0x93 = 10 01 00 11
      db_alpha_to_mask = db_alpha_to_mask
         .ALPHA_TO_MASK_OFFSET0(3)
         .ALPHA_TO_MASK_OFFSET1(0)
         .ALPHA_TO_MASK_OFFSET2(1)
         .ALPHA_TO_MASK_OFFSET3(2);
      break;
   }

   reg->db_alpha_to_mask = db_alpha_to_mask;
}

void
GX2GetAlphaToMaskReg(const GX2AlphaToMaskReg *reg,
                     be_val<BOOL> *alphaToMask,
                     be_val<GX2AlphaToMaskMode> *mode)
{
   auto db_alpha_to_mask = reg->db_alpha_to_mask.value();
   auto value = (db_alpha_to_mask.value >> 8) & 0xff;
   *alphaToMask = db_alpha_to_mask.ALPHA_TO_MASK_ENABLE();

   switch (value) {
   case 0x78:
      *mode = GX2AlphaToMaskMode::Dither0;
      break;
   case 0xC6:
      *mode = GX2AlphaToMaskMode::Dither90;
      break;
   case 0x2D:
      *mode = GX2AlphaToMaskMode::Dither180;
      break;
   case 0x93:
      *mode = GX2AlphaToMaskMode::Dither270;
      break;
   default:
      *mode = GX2AlphaToMaskMode::NonDithered;
      break;
   }
}

void
GX2SetAlphaToMaskReg(GX2AlphaToMaskReg *reg)
{
   auto db_alpha_to_mask = reg->db_alpha_to_mask.value();
   pm4::write(pm4::SetContextReg { latte::Register::DB_ALPHA_TO_MASK, db_alpha_to_mask.value });
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
GX2GetBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                            be_val<float> *red,
                            be_val<float> *green,
                            be_val<float> *blue,
                            be_val<float> *alpha)
{
   *red = reg->red;
   *green = reg->green;
   *blue = reg->blue;
   *alpha = reg->alpha;
}

void
GX2SetBlendConstantColorReg(GX2BlendConstantColorReg *reg)
{
   float colors[] = {
      reg->red,
      reg->green,
      reg->blue,
      reg->alpha
   };

   auto values = reinterpret_cast<uint32_t *>(colors);
   pm4::write(pm4::SetContextRegs { latte::Register::CB_BLEND_RED, gsl::as_span(values, 4) });
}

void
GX2SetBlendControl(GX2RenderTarget target,
                   GX2BlendMode colorSrcBlend,
                   GX2BlendMode colorDstBlend,
                   GX2BlendCombineMode colorCombine,
                   BOOL useAlphaBlend,
                   GX2BlendMode alphaSrcBlend,
                   GX2BlendMode alphaDstBlend,
                   GX2BlendCombineMode alphaCombine)
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
GX2InitBlendControlReg(GX2BlendControlReg *reg,
                       GX2RenderTarget target,
                       GX2BlendMode colorSrcBlend,
                       GX2BlendMode colorDstBlend,
                       GX2BlendCombineMode colorCombine,
                       BOOL useAlphaBlend,
                       GX2BlendMode alphaSrcBlend,
                       GX2BlendMode alphaDstBlend,
                       GX2BlendCombineMode alphaCombine)
{
   auto cb_blend_control = reg->cb_blend_control.value();
   reg->target = target;

   cb_blend_control = cb_blend_control
      .COLOR_SRCBLEND(static_cast<latte::CB_BLEND_FUNC>(colorSrcBlend))
      .COLOR_DESTBLEND(static_cast<latte::CB_BLEND_FUNC>(colorDstBlend))
      .COLOR_COMB_FCN(static_cast<latte::CB_COMB_FUNC>(colorCombine))
      .SEPARATE_ALPHA_BLEND(useAlphaBlend)
      .ALPHA_SRCBLEND(static_cast<latte::CB_BLEND_FUNC>(alphaSrcBlend))
      .ALPHA_DESTBLEND(static_cast<latte::CB_BLEND_FUNC>(alphaDstBlend))
      .ALPHA_COMB_FCN(static_cast<latte::CB_COMB_FUNC>(alphaCombine));

   reg->cb_blend_control = cb_blend_control;
}

void
GX2GetBlendControlReg(GX2BlendControlReg *reg,
                      be_val<GX2RenderTarget> *target,
                      be_val<GX2BlendMode> *colorSrcBlend,
                      be_val<GX2BlendMode> *colorDstBlend,
                      be_val<GX2BlendCombineMode> *colorCombine,
                      be_val<BOOL> *useAlphaBlend,
                      be_val<GX2BlendMode> *alphaSrcBlend,
                      be_val<GX2BlendMode> *alphaDstBlend,
                      be_val<GX2BlendCombineMode> *alphaCombine)
{
   auto cb_blend_control = reg->cb_blend_control.value();
   *target = reg->target;
   *colorSrcBlend = static_cast<GX2BlendMode>(cb_blend_control.COLOR_SRCBLEND());
   *colorDstBlend = static_cast<GX2BlendMode>(cb_blend_control.COLOR_DESTBLEND());
   *colorCombine = static_cast<GX2BlendCombineMode>(cb_blend_control.COLOR_COMB_FCN());
   *useAlphaBlend = cb_blend_control.SEPARATE_ALPHA_BLEND();
   *alphaSrcBlend = static_cast<GX2BlendMode>(cb_blend_control.ALPHA_SRCBLEND());
   *alphaDstBlend = static_cast<GX2BlendMode>(cb_blend_control.ALPHA_DESTBLEND());
   *alphaCombine = static_cast<GX2BlendCombineMode>(cb_blend_control.ALPHA_COMB_FCN());
}

void
GX2SetBlendControlReg(GX2BlendControlReg *reg)
{
   auto cb_blend_control = reg->cb_blend_control.value();
   auto id = static_cast<latte::Register>(latte::Register::CB_BLEND0_CONTROL + reg->target * 4);
   pm4::write(pm4::SetContextReg { id, cb_blend_control.value });
}

void
GX2SetColorControl(GX2LogicOp rop3,
                   uint8_t targetBlendEnable,
                   BOOL multiWriteEnable,
                   BOOL colorWriteEnable)
{
   GX2ColorControlReg reg;
   GX2InitColorControlReg(&reg,
                          rop3,
                          targetBlendEnable,
                          multiWriteEnable,
                          colorWriteEnable);
   GX2SetColorControlReg(&reg);
}

void
GX2InitColorControlReg(GX2ColorControlReg *reg,
                       GX2LogicOp rop3,
                       uint8_t targetBlendEnable,
                       BOOL multiWriteEnable,
                       BOOL colorWriteEnable)
{
   auto cb_color_control = reg->cb_color_control.value();
   auto specialOp = latte::CB_SPECIAL_DISABLE;

   if (colorWriteEnable) {
      specialOp = latte::CB_SPECIAL_NORMAL;
   }

   cb_color_control = cb_color_control
      .ROP3(rop3)
      .TARGET_BLEND_ENABLE(targetBlendEnable)
      .MULTIWRITE_ENABLE(multiWriteEnable)
      .SPECIAL_OP(specialOp);

   reg->cb_color_control = cb_color_control;
}

void
GX2GetColorControlReg(GX2ColorControlReg *reg,
                      be_val<GX2LogicOp> *rop3,
                      be_val<uint8_t> *targetBlendEnable,
                      be_val<BOOL> *multiWriteEnable,
                      be_val<BOOL> *colorWriteEnable)
{
   auto cb_color_control = reg->cb_color_control.value();

   *rop3 = static_cast<GX2LogicOp>(cb_color_control.ROP3());
   *targetBlendEnable = cb_color_control.TARGET_BLEND_ENABLE();
   *multiWriteEnable = cb_color_control.MULTIWRITE_ENABLE();

   if (cb_color_control.SPECIAL_OP() == latte::CB_SPECIAL_DISABLE) {
      *colorWriteEnable = FALSE;
   } else {
      *colorWriteEnable = TRUE;
   }
}

void
GX2SetColorControlReg(GX2ColorControlReg *reg)
{
   auto cb_color_control = reg->cb_color_control.value();
   pm4::write(pm4::SetContextReg { latte::Register::CB_COLOR_CONTROL, cb_color_control.value });
}

void
GX2SetDepthOnlyControl(BOOL depthTest,
                       BOOL depthWrite,
                       GX2CompareFunction depthCompare)
{
   GX2SetDepthStencilControl(depthTest,
                             depthWrite,
                             depthCompare,
                             FALSE,
                             FALSE,
                             GX2CompareFunction::Never,
                             GX2StencilFunction::Keep,
                             GX2StencilFunction::Keep,
                             GX2StencilFunction::Keep,
                             GX2CompareFunction::Never,
                             GX2StencilFunction::Keep,
                             GX2StencilFunction::Keep,
                             GX2StencilFunction::Keep);
}

void
GX2SetDepthStencilControl(BOOL depthTest,
                          BOOL depthWrite,
                          GX2CompareFunction depthCompare,
                          BOOL stencilTest,
                          BOOL backfaceStencil,
                          GX2CompareFunction frontStencilFunc,
                          GX2StencilFunction frontStencilZPass,
                          GX2StencilFunction frontStencilZFail,
                          GX2StencilFunction frontStencilFail,
                          GX2CompareFunction backStencilFunc,
                          GX2StencilFunction backStencilZPass,
                          GX2StencilFunction backStencilZFail,
                          GX2StencilFunction backStencilFail)
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
GX2InitDepthStencilControlReg(GX2DepthStencilControlReg *reg,
                              BOOL depthTest,
                              BOOL depthWrite,
                              GX2CompareFunction depthCompare,
                              BOOL stencilTest,
                              BOOL backfaceStencil,
                              GX2CompareFunction frontStencilFunc,
                              GX2StencilFunction frontStencilZPass,
                              GX2StencilFunction frontStencilZFail,
                              GX2StencilFunction frontStencilFail,
                              GX2CompareFunction backStencilFunc,
                              GX2StencilFunction backStencilZPass,
                              GX2StencilFunction backStencilZFail,
                              GX2StencilFunction backStencilFail)
{
   auto db_depth_control = reg->db_depth_control.value();

   db_depth_control = db_depth_control
      .Z_ENABLE(!!depthTest)
      .Z_WRITE_ENABLE(!!depthWrite)
      .ZFUNC(static_cast<latte::REF_FUNC>(depthCompare))
      .STENCIL_ENABLE(!!stencilTest)
      .BACKFACE_ENABLE(!!backfaceStencil)
      .STENCILFUNC(static_cast<latte::REF_FUNC>(frontStencilFunc))
      .STENCILZPASS(static_cast<latte::DB_STENCIL_FUNC>(frontStencilZPass))
      .STENCILZFAIL(static_cast<latte::DB_STENCIL_FUNC>(frontStencilZFail))
      .STENCILFAIL(static_cast<latte::DB_STENCIL_FUNC>(frontStencilFail))
      .STENCILFUNC_BF(static_cast<latte::REF_FUNC>(backStencilFunc))
      .STENCILZPASS_BF(static_cast<latte::DB_STENCIL_FUNC>(backStencilZPass))
      .STENCILZFAIL_BF(static_cast<latte::DB_STENCIL_FUNC>(backStencilZFail))
      .STENCILFAIL_BF(static_cast<latte::DB_STENCIL_FUNC>(backStencilFail));

   reg->db_depth_control = db_depth_control;
}

void
GX2GetDepthStencilControlReg(GX2DepthStencilControlReg *reg,
                             be_val<BOOL> *depthTest,
                             be_val<BOOL> *depthWrite,
                             be_val<GX2CompareFunction> *depthCompare,
                             be_val<BOOL> *stencilTest,
                             be_val<BOOL> *backfaceStencil,
                             be_val<GX2CompareFunction> *frontStencilFunc,
                             be_val<GX2StencilFunction> *frontStencilZPass,
                             be_val<GX2StencilFunction> *frontStencilZFail,
                             be_val<GX2StencilFunction> *frontStencilFail,
                             be_val<GX2CompareFunction> *backStencilFunc,
                             be_val<GX2StencilFunction> *backStencilZPass,
                             be_val<GX2StencilFunction> *backStencilZFail,
                             be_val<GX2StencilFunction> *backStencilFail)
{
   auto db_depth_control = reg->db_depth_control.value();
   *depthTest = db_depth_control.Z_ENABLE();
   *depthWrite = db_depth_control.Z_WRITE_ENABLE();
   *depthCompare = static_cast<GX2CompareFunction>(db_depth_control.ZFUNC());
   *stencilTest = db_depth_control.STENCIL_ENABLE();
   *backfaceStencil = db_depth_control.BACKFACE_ENABLE();
   *frontStencilFunc = static_cast<GX2CompareFunction>(db_depth_control.STENCILFUNC());
   *frontStencilZPass = static_cast<GX2StencilFunction>(db_depth_control.STENCILZPASS());
   *frontStencilZFail = static_cast<GX2StencilFunction>(db_depth_control.STENCILZFAIL());
   *frontStencilFail = static_cast<GX2StencilFunction>(db_depth_control.STENCILFAIL());
   *backStencilFunc = static_cast<GX2CompareFunction>(db_depth_control.STENCILFUNC_BF());
   *backStencilZPass = static_cast<GX2StencilFunction>(db_depth_control.STENCILZPASS_BF());
   *backStencilZFail = static_cast<GX2StencilFunction>(db_depth_control.STENCILZFAIL_BF());
   *backStencilFail = static_cast<GX2StencilFunction>(db_depth_control.STENCILFAIL_BF());
}

void
GX2SetDepthStencilControlReg(GX2DepthStencilControlReg *reg)
{
   auto db_depth_control = reg->db_depth_control.value();
   pm4::write(pm4::SetContextReg { latte::Register::DB_DEPTH_CONTROL, db_depth_control.value });
}

void GX2SetStencilMask(uint8_t frontMask,
                       uint8_t frontWriteMask,
                       uint8_t frontRef,
                       uint8_t backMask,
                       uint8_t backWriteMask,
                       uint8_t backRef)
{
   GX2StencilMaskReg reg;
   GX2InitStencilMaskReg(&reg, frontMask, frontWriteMask, frontRef, backMask, backWriteMask, backRef);
   GX2SetStencilMaskReg(&reg);
}

void GX2InitStencilMaskReg(GX2StencilMaskReg *reg,
                           uint8_t frontMask,
                           uint8_t frontWriteMask,
                           uint8_t frontRef,
                           uint8_t backMask,
                           uint8_t backWriteMask,
                           uint8_t backRef)
{
   auto db_stencilrefmask = reg->db_stencilrefmask.value();
   auto db_stencilrefmask_bf = reg->db_stencilrefmask_bf.value();

   db_stencilrefmask = db_stencilrefmask
      .STENCILREF(frontRef)
      .STENCILMASK(frontMask)
      .STENCILWRITEMASK(frontWriteMask);

   db_stencilrefmask_bf = db_stencilrefmask_bf
      .STENCILREF_BF(backRef)
      .STENCILMASK_BF(backMask)
      .STENCILWRITEMASK_BF(backWriteMask);

   reg->db_stencilrefmask = db_stencilrefmask;
   reg->db_stencilrefmask_bf = db_stencilrefmask_bf;
}

void GX2GetStencilMaskReg(GX2StencilMaskReg *reg,
                          uint8_t *frontMask,
                          uint8_t *frontWriteMask,
                          uint8_t *frontRef,
                          uint8_t *backMask,
                          uint8_t *backWriteMask,
                          uint8_t *backRef)
{
   auto db_stencilrefmask = reg->db_stencilrefmask.value();
   auto db_stencilrefmask_bf = reg->db_stencilrefmask_bf.value();

   *frontRef = db_stencilrefmask.STENCILREF();
   *frontMask = db_stencilrefmask.STENCILMASK();
   *frontWriteMask = db_stencilrefmask.STENCILWRITEMASK();

   *backRef = db_stencilrefmask_bf.STENCILREF_BF();
   *backMask = db_stencilrefmask_bf.STENCILMASK_BF();
   *backWriteMask = db_stencilrefmask_bf.STENCILWRITEMASK_BF();
}

void GX2SetStencilMaskReg(GX2StencilMaskReg *reg)
{
   auto db_stencilrefmask = reg->db_stencilrefmask.value();
   auto db_stencilrefmask_bf = reg->db_stencilrefmask_bf.value();
   pm4::write(pm4::SetContextReg { latte::Register::DB_STENCILREFMASK, db_stencilrefmask.value });
   pm4::write(pm4::SetContextReg { latte::Register::DB_STENCILREFMASK_BF, db_stencilrefmask_bf.value });
}

void
GX2SetLineWidth(float width)
{
   GX2LineWidthReg reg;
   GX2InitLineWidthReg(&reg, width);
   GX2SetLineWidthReg(&reg);
}

void
GX2InitLineWidthReg(GX2LineWidthReg *reg,
                    float width)
{
   auto pa_su_line_cntl = reg->pa_su_line_cntl.value();

   pa_su_line_cntl = pa_su_line_cntl
      .WIDTH(gsl::narrow_cast<uint32_t>(width * 8.0f));

   reg->pa_su_line_cntl = pa_su_line_cntl;
}

void
GX2GetLineWidthReg(GX2LineWidthReg *reg,
                   be_val<float> *width)
{
   auto pa_su_line_cntl = reg->pa_su_line_cntl.value();
   *width = static_cast<float>(pa_su_line_cntl.WIDTH()) / 8.0f;
}

void
GX2SetLineWidthReg(GX2LineWidthReg *reg)
{
   auto pa_su_line_cntl = reg->pa_su_line_cntl.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_LINE_CNTL, pa_su_line_cntl.value });
}

void
GX2SetPointSize(float width,
                float height)
{
   GX2PointSizeReg reg;
   GX2InitPointSizeReg(&reg, width, height);
   GX2SetPointSizeReg(&reg);
}

void
GX2InitPointSizeReg(GX2PointSizeReg *reg,
                    float width,
                    float height)
{
   auto pa_su_point_size = reg->pa_su_point_size.value();

   pa_su_point_size = pa_su_point_size
      .WIDTH(gsl::narrow_cast<uint32_t>(width * 8.0f))
      .HEIGHT(gsl::narrow_cast<uint32_t>(height * 8.0f));

   reg->pa_su_point_size = pa_su_point_size;
}

void
GX2GetPointSizeReg(GX2PointSizeReg *reg,
                   be_val<float> *width,
                   be_val<float> *height)
{
   auto pa_su_point_size = reg->pa_su_point_size.value();
   *width = static_cast<float>(pa_su_point_size.WIDTH()) / 8.0f;
   *height = static_cast<float>(pa_su_point_size.HEIGHT()) / 8.0f;
}

void
GX2SetPointSizeReg(GX2PointSizeReg *reg)
{
   auto pa_su_point_size = reg->pa_su_point_size.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_POINT_SIZE, pa_su_point_size.value });
}

void
GX2SetPointLimits(float min,
                  float max)
{
   GX2PointLimitsReg reg;
   GX2InitPointLimitsReg(&reg, min, max);
   GX2SetPointLimitsReg(&reg);
}

void
GX2InitPointLimitsReg(GX2PointLimitsReg *reg,
                      float min,
                      float max)
{
   auto pa_su_point_minmax = reg->pa_su_point_minmax.value();

   pa_su_point_minmax = pa_su_point_minmax
      .MIN_SIZE(gsl::narrow_cast<uint32_t>(min * 8.0f))
      .MAX_SIZE(gsl::narrow_cast<uint32_t>(max * 8.0f));

   reg->pa_su_point_minmax = pa_su_point_minmax;
}

void
GX2GetPointLimitsReg(GX2PointLimitsReg *reg,
                     be_val<float> *min,
                     be_val<float> *max)
{
   auto pa_su_point_minmax = reg->pa_su_point_minmax.value();
   *min = static_cast<float>(pa_su_point_minmax.MIN_SIZE()) / 8.0f;
   *max = static_cast<float>(pa_su_point_minmax.MAX_SIZE()) / 8.0f;
}

void
GX2SetPointLimitsReg(GX2PointLimitsReg *reg)
{
   auto pa_su_point_minmax = reg->pa_su_point_minmax.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_POINT_MINMAX, pa_su_point_minmax.value });
}

void
GX2SetCullOnlyControl(GX2FrontFace frontFace,
                      BOOL cullFront,
                      BOOL cullBack)
{
   GX2SetPolygonControl(frontFace,
                        cullFront,
                        cullBack,
                        FALSE,
                        GX2PolygonMode::Point,
                        GX2PolygonMode::Point,
                        FALSE,
                        FALSE,
                        FALSE);
}

void
GX2SetPolygonControl(GX2FrontFace frontFace,
                     BOOL cullFront,
                     BOOL cullBack,
                     BOOL polyMode,
                     GX2PolygonMode polyModeFront,
                     GX2PolygonMode polyModeBack,
                     BOOL polyOffsetFrontEnable,
                     BOOL polyOffsetBackEnable,
                     BOOL polyOffsetParaEnable)
{
   GX2PolygonControlReg reg;
   GX2InitPolygonControlReg(&reg,
                            frontFace,
                            cullFront,
                            cullBack,
                            polyMode,
                            polyModeFront,
                            polyModeBack,
                            polyOffsetFrontEnable,
                            polyOffsetBackEnable,
                            polyOffsetParaEnable);
   GX2SetPolygonControlReg(&reg);
}

void
GX2InitPolygonControlReg(GX2PolygonControlReg *reg,
                         GX2FrontFace frontFace,
                         BOOL cullFront,
                         BOOL cullBack,
                         BOOL polyMode,
                         GX2PolygonMode polyModeFront,
                         GX2PolygonMode polyModeBack,
                         BOOL polyOffsetFrontEnable,
                         BOOL polyOffsetBackEnable,
                         BOOL polyOffsetParaEnable)
{
   auto pa_su_sc_mode_cntl = reg->pa_su_sc_mode_cntl.value();

   pa_su_sc_mode_cntl = pa_su_sc_mode_cntl
      .FACE(static_cast<latte::PA_FACE>(frontFace))
      .CULL_FRONT(!!cullFront)
      .CULL_BACK(!!cullBack)
      .POLY_MODE(!!polyMode)
      .POLYMODE_FRONT_PTYPE(polyModeFront)
      .POLYMODE_BACK_PTYPE(polyModeBack)
      .POLY_OFFSET_FRONT_ENABLE(!!polyOffsetFrontEnable)
      .POLY_OFFSET_BACK_ENABLE(!!polyOffsetBackEnable)
      .POLY_OFFSET_PARA_ENABLE(!!polyOffsetParaEnable);

   reg->pa_su_sc_mode_cntl = pa_su_sc_mode_cntl;
}

void
GX2GetPolygonControlReg(GX2PolygonControlReg *reg,
                        be_val<GX2FrontFace> *frontFace,
                        be_val<BOOL> *cullFront,
                        be_val<BOOL> *cullBack,
                        be_val<BOOL> *polyMode,
                        be_val<GX2PolygonMode> *polyModeFront,
                        be_val<GX2PolygonMode> *polyModeBack,
                        be_val<BOOL> *polyOffsetFrontEnable,
                        be_val<BOOL> *polyOffsetBackEnable,
                        be_val<BOOL> *polyOffsetParaEnable)
{
   auto pa_su_sc_mode_cntl = reg->pa_su_sc_mode_cntl.value();
   *frontFace = static_cast<GX2FrontFace>(pa_su_sc_mode_cntl.FACE());
   *cullFront = pa_su_sc_mode_cntl.CULL_FRONT();
   *cullBack = pa_su_sc_mode_cntl.CULL_BACK();
   *polyMode = pa_su_sc_mode_cntl.POLY_MODE();
   *polyModeFront = static_cast<GX2PolygonMode>(pa_su_sc_mode_cntl.POLYMODE_FRONT_PTYPE());
   *polyModeBack = static_cast<GX2PolygonMode>(pa_su_sc_mode_cntl.POLYMODE_BACK_PTYPE());
   *polyOffsetFrontEnable = pa_su_sc_mode_cntl.POLY_OFFSET_FRONT_ENABLE();
   *polyOffsetBackEnable = pa_su_sc_mode_cntl.POLY_OFFSET_BACK_ENABLE();
   *polyOffsetParaEnable = pa_su_sc_mode_cntl.POLY_OFFSET_PARA_ENABLE();
}

void
GX2SetPolygonControlReg(GX2PolygonControlReg *reg)
{
   auto pa_su_sc_mode_cntl = reg->pa_su_sc_mode_cntl.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_SC_MODE_CNTL, pa_su_sc_mode_cntl.value });
}

void
GX2SetPolygonOffset(float frontOffset,
                    float frontScale,
                    float backOffset,
                    float backScale,
                    float clamp)
{
   GX2PolygonOffsetReg reg;
   GX2InitPolygonOffsetReg(&reg,
                           frontOffset,
                           frontScale,
                           backOffset,
                           backScale,
                           clamp);
   GX2SetPolygonOffsetReg(&reg);
}

void
GX2InitPolygonOffsetReg(GX2PolygonOffsetReg *reg,
                        float frontOffset,
                        float frontScale,
                        float backOffset,
                        float backScale,
                        float clamp)
{
   auto pa_su_poly_offset_front_offset = reg->pa_su_poly_offset_front_offset.value();
   auto pa_su_poly_offset_front_scale = reg->pa_su_poly_offset_front_scale.value();
   auto pa_su_poly_offset_back_offset = reg->pa_su_poly_offset_back_offset.value();
   auto pa_su_poly_offset_back_scale = reg->pa_su_poly_offset_back_scale.value();
   auto pa_su_poly_offset_clamp = reg->pa_su_poly_offset_clamp.value();

   pa_su_poly_offset_front_offset.OFFSET = frontOffset;
   pa_su_poly_offset_front_scale.SCALE = frontScale * 16.0f;
   pa_su_poly_offset_back_offset.OFFSET = backOffset;
   pa_su_poly_offset_back_scale.SCALE = backScale * 16.0f;
   pa_su_poly_offset_clamp.CLAMP = clamp;

   reg->pa_su_poly_offset_front_offset = pa_su_poly_offset_front_offset;
   reg->pa_su_poly_offset_front_scale = pa_su_poly_offset_front_scale;
   reg->pa_su_poly_offset_back_offset = pa_su_poly_offset_back_offset;
   reg->pa_su_poly_offset_back_scale = pa_su_poly_offset_back_scale;
   reg->pa_su_poly_offset_clamp = pa_su_poly_offset_clamp;
}

void
GX2GetPolygonOffsetReg(GX2PolygonOffsetReg *reg,
                       be_val<float> *frontOffset,
                       be_val<float> *frontScale,
                       be_val<float> *backOffset,
                       be_val<float> *backScale,
                       be_val<float> *clamp)
{
   auto pa_su_poly_offset_front_offset = reg->pa_su_poly_offset_front_offset.value();
   auto pa_su_poly_offset_front_scale = reg->pa_su_poly_offset_front_scale.value();
   auto pa_su_poly_offset_back_offset = reg->pa_su_poly_offset_back_offset.value();
   auto pa_su_poly_offset_back_scale = reg->pa_su_poly_offset_back_scale.value();
   auto pa_su_poly_offset_clamp = reg->pa_su_poly_offset_clamp.value();

   *frontOffset = pa_su_poly_offset_front_offset.OFFSET;
   *frontScale = pa_su_poly_offset_front_scale.SCALE / 16.0f;
   *backOffset = pa_su_poly_offset_back_offset.OFFSET;
   *backScale = pa_su_poly_offset_back_scale.SCALE / 16.0f;
   *clamp = pa_su_poly_offset_clamp.CLAMP;
}

void
GX2SetPolygonOffsetReg(GX2PolygonOffsetReg *reg)
{
   auto pa_su_poly_offset_front_offset = reg->pa_su_poly_offset_front_offset.value();
   auto pa_su_poly_offset_front_scale = reg->pa_su_poly_offset_front_scale.value();
   auto pa_su_poly_offset_back_offset = reg->pa_su_poly_offset_back_offset.value();
   auto pa_su_poly_offset_back_scale = reg->pa_su_poly_offset_back_scale.value();

   uint32_t values[] = {
      pa_su_poly_offset_front_scale.value,
      pa_su_poly_offset_front_offset.value,
      pa_su_poly_offset_back_scale.value,
      pa_su_poly_offset_back_offset.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::PA_SU_POLY_OFFSET_FRONT_SCALE, gsl::as_span(values) });

   auto pa_su_poly_offset_clamp = reg->pa_su_poly_offset_clamp.value();
   pm4::write(pm4::SetContextReg { latte::Register::PA_SU_POLY_OFFSET_CLAMP, pa_su_poly_offset_clamp.value });
}

void
GX2SetScissor(uint32_t x,
              uint32_t y,
              uint32_t width,
              uint32_t height)
{
   GX2ScissorReg reg;
   GX2InitScissorReg(&reg, x, y, width, height);
   GX2SetScissorReg(&reg);
}

void
GX2InitScissorReg(GX2ScissorReg *reg,
                  uint32_t x,
                  uint32_t y,
                  uint32_t width,
                  uint32_t height)
{
   auto pa_sc_generic_scissor_tl = reg->pa_sc_generic_scissor_tl.value();
   auto pa_sc_generic_scissor_br = reg->pa_sc_generic_scissor_br.value();

   pa_sc_generic_scissor_tl = pa_sc_generic_scissor_tl
      .TL_X(x)
      .TL_Y(y);

   pa_sc_generic_scissor_br = pa_sc_generic_scissor_br
      .BR_X(x + width)
      .BR_Y(y + height);

   reg->pa_sc_generic_scissor_tl = pa_sc_generic_scissor_tl;
   reg->pa_sc_generic_scissor_br = pa_sc_generic_scissor_br;
}

void
GX2GetScissorReg(GX2ScissorReg *reg,
                 be_val<uint32_t> *x,
                 be_val<uint32_t> *y,
                 be_val<uint32_t> *width,
                 be_val<uint32_t> *height)
{
   auto pa_sc_generic_scissor_tl = reg->pa_sc_generic_scissor_tl.value();
   auto pa_sc_generic_scissor_br = reg->pa_sc_generic_scissor_br.value();

   *x = pa_sc_generic_scissor_tl.TL_X();
   *y = pa_sc_generic_scissor_tl.TL_Y();
   *width = pa_sc_generic_scissor_br.BR_X() - pa_sc_generic_scissor_tl.TL_X();
   *height = pa_sc_generic_scissor_br.BR_Y() - pa_sc_generic_scissor_tl.TL_Y();
}

void
GX2SetScissorReg(GX2ScissorReg *reg)
{
   auto pa_sc_generic_scissor_tl = reg->pa_sc_generic_scissor_tl.value();
   auto pa_sc_generic_scissor_br = reg->pa_sc_generic_scissor_br.value();

   uint32_t values[] = {
      pa_sc_generic_scissor_tl.value,
      pa_sc_generic_scissor_br.value,
   };

   pm4::write(pm4::SetContextRegs { latte::Register::PA_SC_GENERIC_SCISSOR_TL, gsl::as_span(values) });
}

void
GX2SetTargetChannelMasks(GX2ChannelMask mask0,
                         GX2ChannelMask mask1,
                         GX2ChannelMask mask2,
                         GX2ChannelMask mask3,
                         GX2ChannelMask mask4,
                         GX2ChannelMask mask5,
                         GX2ChannelMask mask6,
                         GX2ChannelMask mask7)
{
   GX2TargetChannelMaskReg reg;
   GX2InitTargetChannelMasksReg(&reg,
                                mask0,
                                mask1,
                                mask2,
                                mask3,
                                mask4,
                                mask5,
                                mask6,
                                mask7);

   GX2SetTargetChannelMasksReg(&reg);
}

void
GX2InitTargetChannelMasksReg(GX2TargetChannelMaskReg *reg,
                             GX2ChannelMask mask0,
                             GX2ChannelMask mask1,
                             GX2ChannelMask mask2,
                             GX2ChannelMask mask3,
                             GX2ChannelMask mask4,
                             GX2ChannelMask mask5,
                             GX2ChannelMask mask6,
                             GX2ChannelMask mask7)
{
   auto cb_target_mask = reg->cb_target_mask.value();

   cb_target_mask = cb_target_mask
      .TARGET0_ENABLE(mask0)
      .TARGET1_ENABLE(mask1)
      .TARGET2_ENABLE(mask2)
      .TARGET3_ENABLE(mask3)
      .TARGET4_ENABLE(mask4)
      .TARGET5_ENABLE(mask5)
      .TARGET6_ENABLE(mask6)
      .TARGET7_ENABLE(mask7);

   reg->cb_target_mask = cb_target_mask;
}

void
GX2GetTargetChannelMasksReg(GX2TargetChannelMaskReg *reg,
                            be_val<GX2ChannelMask> *mask0,
                            be_val<GX2ChannelMask> *mask1,
                            be_val<GX2ChannelMask> *mask2,
                            be_val<GX2ChannelMask> *mask3,
                            be_val<GX2ChannelMask> *mask4,
                            be_val<GX2ChannelMask> *mask5,
                            be_val<GX2ChannelMask> *mask6,
                            be_val<GX2ChannelMask> *mask7)
{
   auto cb_target_mask = reg->cb_target_mask.value();
   *mask0 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET0_ENABLE());
   *mask1 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET1_ENABLE());
   *mask2 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET2_ENABLE());
   *mask3 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET3_ENABLE());
   *mask4 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET4_ENABLE());
   *mask5 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET5_ENABLE());
   *mask6 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET6_ENABLE());
   *mask7 = static_cast<GX2ChannelMask>(cb_target_mask.TARGET7_ENABLE());
}

void
GX2SetTargetChannelMasksReg(GX2TargetChannelMaskReg *reg)
{
   auto cb_target_mask = reg->cb_target_mask.value();
   pm4::write(pm4::SetContextReg { latte::Register::CB_TARGET_MASK, cb_target_mask.value });
}

void
GX2SetViewport(float x,
               float y,
               float width,
               float height,
               float nearZ,
               float farZ)
{
   GX2ViewportReg reg;
   GX2InitViewportReg(&reg, x, y, width, height, nearZ, farZ);
   GX2SetViewportReg(&reg);
}

void
GX2InitViewportReg(GX2ViewportReg *reg,
                   float x,
                   float y,
                   float width,
                   float height,
                   float nearZ,
                   float farZ)
{
   auto pa_cl_vport_xscale = reg->pa_cl_vport_xscale.value();
   auto pa_cl_vport_xoffset = reg->pa_cl_vport_xoffset.value();
   auto pa_cl_vport_yscale = reg->pa_cl_vport_yscale.value();
   auto pa_cl_vport_yoffset = reg->pa_cl_vport_yoffset.value();
   auto pa_cl_vport_zscale = reg->pa_cl_vport_zscale.value();
   auto pa_cl_vport_zoffset = reg->pa_cl_vport_zoffset.value();

   auto pa_cl_gb_vert_clip_adj = reg->pa_cl_gb_vert_clip_adj.value();
   auto pa_cl_gb_vert_disc_adj = reg->pa_cl_gb_vert_disc_adj.value();
   auto pa_cl_gb_horz_clip_adj = reg->pa_cl_gb_horz_clip_adj.value();
   auto pa_cl_gb_horz_disc_adj = reg->pa_cl_gb_horz_disc_adj.value();

   auto pa_sc_vport_zmin = reg->pa_sc_vport_zmin.value();
   auto pa_sc_vport_zmax = reg->pa_sc_vport_zmax.value();

   pa_cl_vport_xscale.VPORT_XSCALE = width * 0.5f;
   pa_cl_vport_xoffset.VPORT_XOFFSET = x + (width * 0.5f);
   pa_cl_vport_yscale.VPORT_YSCALE = height * 0.5f;
   pa_cl_vport_yoffset.VPORT_YOFFSET = y + (height * 0.5f);
   pa_cl_vport_zscale.VPORT_ZSCALE = (farZ - nearZ) * 0.5f;
   pa_cl_vport_zoffset.VPORT_ZOFFSET = (farZ + nearZ) * 0.5f;

   pa_cl_gb_vert_clip_adj.DATA_REGISTER = 1.0f;
   pa_cl_gb_vert_disc_adj.DATA_REGISTER = 1.0f;
   pa_cl_gb_horz_clip_adj.DATA_REGISTER = 1.0f;
   pa_cl_gb_horz_disc_adj.DATA_REGISTER = 1.0f;

   pa_sc_vport_zmin.VPORT_ZMIN = std::min(nearZ, farZ);
   pa_sc_vport_zmax.VPORT_ZMAX = std::max(nearZ, farZ);

   reg->pa_cl_vport_xscale = pa_cl_vport_xscale;
   reg->pa_cl_vport_xoffset = pa_cl_vport_xoffset;
   reg->pa_cl_vport_yscale = pa_cl_vport_yscale;
   reg->pa_cl_vport_yoffset = pa_cl_vport_yoffset;
   reg->pa_cl_vport_zscale = pa_cl_vport_zscale;
   reg->pa_cl_vport_zoffset = pa_cl_vport_zoffset;

   reg->pa_cl_gb_vert_clip_adj = pa_cl_gb_vert_clip_adj;
   reg->pa_cl_gb_vert_disc_adj = pa_cl_gb_vert_disc_adj;
   reg->pa_cl_gb_horz_clip_adj = pa_cl_gb_horz_clip_adj;
   reg->pa_cl_gb_horz_disc_adj = pa_cl_gb_horz_disc_adj;

   reg->pa_sc_vport_zmin = pa_sc_vport_zmin;
   reg->pa_sc_vport_zmax = pa_sc_vport_zmax;
}

void
GX2GetViewportReg(GX2ViewportReg *reg,
                  be_val<float> *x,
                  be_val<float> *y,
                  be_val<float> *width,
                  be_val<float> *height,
                  be_val<float> *nearZ,
                  be_val<float> *farZ)
{
   auto pa_cl_vport_xscale = reg->pa_cl_vport_xscale.value();
   auto pa_cl_vport_xoffset = reg->pa_cl_vport_xoffset.value();
   auto pa_cl_vport_yscale = reg->pa_cl_vport_yscale.value();
   auto pa_cl_vport_yoffset = reg->pa_cl_vport_yoffset.value();
   auto pa_cl_vport_zscale = reg->pa_cl_vport_zscale.value();
   auto pa_cl_vport_zoffset = reg->pa_cl_vport_zoffset.value();

   auto pa_cl_gb_vert_clip_adj = reg->pa_cl_gb_vert_clip_adj.value();
   auto pa_cl_gb_vert_disc_adj = reg->pa_cl_gb_vert_disc_adj.value();
   auto pa_cl_gb_horz_clip_adj = reg->pa_cl_gb_horz_clip_adj.value();
   auto pa_cl_gb_horz_disc_adj = reg->pa_cl_gb_horz_disc_adj.value();

   auto pa_sc_vport_zmin = reg->pa_sc_vport_zmin.value();
   auto pa_sc_vport_zmax = reg->pa_sc_vport_zmax.value();

   *x = pa_cl_vport_xoffset.VPORT_XOFFSET - pa_cl_vport_xscale.VPORT_XSCALE;
   *y = pa_cl_vport_yoffset.VPORT_YOFFSET - pa_cl_vport_yscale.VPORT_YSCALE;

   *width = 2.0f * pa_cl_vport_xscale.VPORT_XSCALE;
   *height = 2.0f * pa_cl_vport_yscale.VPORT_YSCALE;

   if (pa_cl_vport_zscale.VPORT_ZSCALE > 0.0f) {
      *nearZ = pa_sc_vport_zmin.VPORT_ZMIN;
      *farZ = pa_sc_vport_zmax.VPORT_ZMAX;
   } else {
      *farZ = pa_sc_vport_zmin.VPORT_ZMIN;
      *nearZ = pa_sc_vport_zmax.VPORT_ZMAX;
   }
}

void
GX2SetViewportReg(GX2ViewportReg *reg)
{
   auto pa_cl_vport_xscale = reg->pa_cl_vport_xscale.value();
   auto pa_cl_vport_xoffset = reg->pa_cl_vport_xoffset.value();
   auto pa_cl_vport_yscale = reg->pa_cl_vport_yscale.value();
   auto pa_cl_vport_yoffset = reg->pa_cl_vport_yoffset.value();
   auto pa_cl_vport_zscale = reg->pa_cl_vport_zscale.value();
   auto pa_cl_vport_zoffset = reg->pa_cl_vport_zoffset.value();
   uint32_t values1[] = {
      pa_cl_vport_xscale.value,
      pa_cl_vport_xoffset.value,
      pa_cl_vport_yscale.value,
      pa_cl_vport_yoffset.value,
      pa_cl_vport_zscale.value,
      pa_cl_vport_zoffset.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::PA_CL_VPORT_XSCALE_0, gsl::as_span(values1) });

   auto pa_cl_gb_vert_clip_adj = reg->pa_cl_gb_vert_clip_adj.value();
   auto pa_cl_gb_vert_disc_adj = reg->pa_cl_gb_vert_disc_adj.value();
   auto pa_cl_gb_horz_clip_adj = reg->pa_cl_gb_horz_clip_adj.value();
   auto pa_cl_gb_horz_disc_adj = reg->pa_cl_gb_horz_disc_adj.value();
   uint32_t values2[] = {
      pa_cl_gb_vert_clip_adj.value,
      pa_cl_gb_vert_disc_adj.value,
      pa_cl_gb_horz_clip_adj.value,
      pa_cl_gb_horz_disc_adj.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::PA_CL_GB_VERT_CLIP_ADJ, gsl::as_span(values2) });

   auto pa_sc_vport_zmin = reg->pa_sc_vport_zmin.value();
   auto pa_sc_vport_zmax = reg->pa_sc_vport_zmax.value();
   uint32_t values3[] = {
      pa_sc_vport_zmin.value,
      pa_sc_vport_zmax.value,
   };
   pm4::write(pm4::SetContextRegs { latte::Register::PA_SC_VPORT_ZMIN_0, gsl::as_span(values3) });
}

void
GX2SetRasterizerClipControl(BOOL rasteriser,
                            BOOL zclipEnable)
{
   GX2SetRasterizerClipControlEx(rasteriser, zclipEnable, FALSE);
}

void
GX2SetRasterizerClipControlEx(BOOL rasteriser,
                              BOOL zclipEnable,
                              BOOL halfZ)
{
   auto pa_cl_clip_cntl = latte::PA_CL_CLIP_CNTL::get(0);

   pa_cl_clip_cntl = pa_cl_clip_cntl
      .RASTERISER_DISABLE(!rasteriser)
      .ZCLIP_NEAR_DISABLE(!zclipEnable)
      .ZCLIP_FAR_DISABLE(!zclipEnable)
      .DX_CLIP_SPACE_DEF(!!halfZ);

   pm4::write(pm4::SetContextReg { latte::Register::PA_CL_CLIP_CNTL, pa_cl_clip_cntl.value });
}

void
GX2SetRasterizerClipControlHalfZ(BOOL rasteriser,
                                 BOOL zclipEnable,
                                 BOOL halfZ)
{
   GX2SetRasterizerClipControlEx(rasteriser, zclipEnable, halfZ);
}

} // namespace gx2
