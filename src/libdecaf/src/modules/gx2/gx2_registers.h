#pragma once
#include "common/types.h"
#include "gpu/latte_registers.h"
#include "gx2_enum.h"
#include "common/structsize.h"

namespace gx2
{

#pragma pack(push, 1)

struct GX2AAMaskReg
{
   be_val<latte::PA_SC_AA_MASK> pa_sc_aa_mask;
};
CHECK_SIZE(GX2AAMaskReg, 4);
CHECK_OFFSET(GX2AAMaskReg, 0, pa_sc_aa_mask);

struct GX2AlphaTestReg
{
   be_val<latte::SX_ALPHA_TEST_CONTROL> sx_alpha_test_control;
   be_val<latte::SX_ALPHA_REF> sx_alpha_ref;
};
CHECK_SIZE(GX2AlphaTestReg, 8);
CHECK_OFFSET(GX2AlphaTestReg, 0, sx_alpha_test_control);
CHECK_OFFSET(GX2AlphaTestReg, 4, sx_alpha_ref);

struct GX2AlphaToMaskReg
{
   be_val<latte::DB_ALPHA_TO_MASK> db_alpha_to_mask;
};
CHECK_SIZE(GX2AlphaToMaskReg, 4);
CHECK_OFFSET(GX2AlphaToMaskReg, 0, db_alpha_to_mask);

struct GX2BlendControlReg
{
   be_val<GX2RenderTarget> target;
   be_val<latte::CB_BLENDN_CONTROL> cb_blend_control;
};
CHECK_SIZE(GX2BlendControlReg, 8);
CHECK_OFFSET(GX2BlendControlReg, 0, target);
CHECK_OFFSET(GX2BlendControlReg, 4, cb_blend_control);

struct GX2BlendConstantColorReg
{
   be_val<float> red;
   be_val<float> green;
   be_val<float> blue;
   be_val<float> alpha;
};
CHECK_SIZE(GX2BlendConstantColorReg, 0x10);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x00, red);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x04, green);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x08, blue);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x0c, alpha);

struct GX2ColorControlReg
{
   be_val<latte::CB_COLOR_CONTROL> cb_color_control;
};
CHECK_SIZE(GX2ColorControlReg, 0x04);
CHECK_OFFSET(GX2ColorControlReg, 0x00, cb_color_control);

struct GX2DepthStencilControlReg
{
   be_val<latte::DB_DEPTH_CONTROL> db_depth_control;
};
CHECK_SIZE(GX2DepthStencilControlReg, 4);
CHECK_OFFSET(GX2DepthStencilControlReg, 0, db_depth_control);

struct GX2StencilMaskReg
{
   be_val<latte::DB_STENCILREFMASK> db_stencilrefmask;
   be_val<latte::DB_STENCILREFMASK_BF> db_stencilrefmask_bf;
};
CHECK_SIZE(GX2StencilMaskReg, 8);
CHECK_OFFSET(GX2StencilMaskReg, 0, db_stencilrefmask);
CHECK_OFFSET(GX2StencilMaskReg, 4, db_stencilrefmask_bf);

struct GX2LineWidthReg
{
   be_val<latte::PA_SU_LINE_CNTL> pa_su_line_cntl;
};
CHECK_SIZE(GX2LineWidthReg, 4);
CHECK_OFFSET(GX2LineWidthReg, 0, pa_su_line_cntl);

struct GX2PointSizeReg
{
   be_val<latte::PA_SU_POINT_SIZE> pa_su_point_size;
};
CHECK_SIZE(GX2PointSizeReg, 4);
CHECK_OFFSET(GX2PointSizeReg, 0, pa_su_point_size);

struct GX2PointLimitsReg
{
   be_val<latte::PA_SU_POINT_MINMAX> pa_su_point_minmax;
};
CHECK_SIZE(GX2PointLimitsReg, 4);
CHECK_OFFSET(GX2PointLimitsReg, 0, pa_su_point_minmax);

struct GX2PolygonControlReg
{
   be_val<latte::PA_SU_SC_MODE_CNTL> pa_su_sc_mode_cntl;
};
CHECK_SIZE(GX2PolygonControlReg, 4);
CHECK_OFFSET(GX2PolygonControlReg, 0, pa_su_sc_mode_cntl);

struct GX2PolygonOffsetReg
{
   be_val<latte::PA_SU_POLY_OFFSET_FRONT_SCALE> pa_su_poly_offset_front_scale;
   be_val<latte::PA_SU_POLY_OFFSET_FRONT_OFFSET> pa_su_poly_offset_front_offset;
   be_val<latte::PA_SU_POLY_OFFSET_BACK_SCALE> pa_su_poly_offset_back_scale;
   be_val<latte::PA_SU_POLY_OFFSET_BACK_OFFSET> pa_su_poly_offset_back_offset;

   be_val<latte::PA_SU_POLY_OFFSET_CLAMP> pa_su_poly_offset_clamp;
};
CHECK_SIZE(GX2PolygonOffsetReg, 0x14);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x00, pa_su_poly_offset_front_scale);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x04, pa_su_poly_offset_front_offset);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x08, pa_su_poly_offset_back_scale);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x0C, pa_su_poly_offset_back_offset);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x10, pa_su_poly_offset_clamp);

struct GX2ScissorReg
{
   be_val<latte::PA_SC_GENERIC_SCISSOR_TL> pa_sc_generic_scissor_tl;
   be_val<latte::PA_SC_GENERIC_SCISSOR_BR> pa_sc_generic_scissor_br;
};
CHECK_SIZE(GX2ScissorReg, 0x08);
CHECK_OFFSET(GX2ScissorReg, 0x00, pa_sc_generic_scissor_tl);
CHECK_OFFSET(GX2ScissorReg, 0x04, pa_sc_generic_scissor_br);

struct GX2TargetChannelMaskReg
{
   be_val<latte::CB_TARGET_MASK> cb_target_mask;
};
CHECK_SIZE(GX2TargetChannelMaskReg, 0x04);
CHECK_OFFSET(GX2TargetChannelMaskReg, 0x00, cb_target_mask);

struct GX2ViewportReg
{
   be_val<latte::PA_CL_VPORT_XSCALE_N> pa_cl_vport_xscale;
   be_val<latte::PA_CL_VPORT_XOFFSET_N> pa_cl_vport_xoffset;
   be_val<latte::PA_CL_VPORT_YSCALE_N> pa_cl_vport_yscale;
   be_val<latte::PA_CL_VPORT_YOFFSET_N> pa_cl_vport_yoffset;
   be_val<latte::PA_CL_VPORT_ZSCALE_N> pa_cl_vport_zscale;
   be_val<latte::PA_CL_VPORT_ZOFFSET_N> pa_cl_vport_zoffset;

   be_val<latte::PA_CL_GB_VERT_CLIP_ADJ> pa_cl_gb_vert_clip_adj;
   be_val<latte::PA_CL_GB_VERT_DISC_ADJ> pa_cl_gb_vert_disc_adj;
   be_val<latte::PA_CL_GB_HORZ_CLIP_ADJ> pa_cl_gb_horz_clip_adj;
   be_val<latte::PA_CL_GB_HORZ_DISC_ADJ> pa_cl_gb_horz_disc_adj;

   be_val<latte::PA_SC_VPORT_ZMIN_N> pa_sc_vport_zmin;
   be_val<latte::PA_SC_VPORT_ZMAX_N> pa_sc_vport_zmax;
};
CHECK_SIZE(GX2ViewportReg, 0x30);
CHECK_OFFSET(GX2ViewportReg, 0x00, pa_cl_vport_xscale);
CHECK_OFFSET(GX2ViewportReg, 0x04, pa_cl_vport_xoffset);
CHECK_OFFSET(GX2ViewportReg, 0x08, pa_cl_vport_yscale);
CHECK_OFFSET(GX2ViewportReg, 0x0C, pa_cl_vport_yoffset);
CHECK_OFFSET(GX2ViewportReg, 0x10, pa_cl_vport_zscale);
CHECK_OFFSET(GX2ViewportReg, 0x14, pa_cl_vport_zoffset);
CHECK_OFFSET(GX2ViewportReg, 0x18, pa_cl_gb_vert_clip_adj);
CHECK_OFFSET(GX2ViewportReg, 0x1C, pa_cl_gb_vert_disc_adj);
CHECK_OFFSET(GX2ViewportReg, 0x20, pa_cl_gb_horz_clip_adj);
CHECK_OFFSET(GX2ViewportReg, 0x24, pa_cl_gb_horz_disc_adj);
CHECK_OFFSET(GX2ViewportReg, 0x28, pa_sc_vport_zmin);
CHECK_OFFSET(GX2ViewportReg, 0x2C, pa_sc_vport_zmax);

#pragma pack(pop)

void
GX2SetAAMask(uint8_t upperLeft,
             uint8_t upperRight,
             uint8_t lowerLeft,
             uint8_t lowerRight);

void
GX2InitAAMaskReg(GX2AAMaskReg *reg,
                 uint8_t upperLeft,
                 uint8_t upperRight,
                 uint8_t lowerLeft,
                 uint8_t lowerRight);

void
GX2GetAAMaskReg(GX2AAMaskReg *reg,
                be_val<uint8_t> *upperLeft,
                be_val<uint8_t> *upperRight,
                be_val<uint8_t> *lowerLeft,
                be_val<uint8_t> *lowerRight);

void
GX2SetAAMaskReg(GX2AAMaskReg *reg);

void
GX2SetAlphaTest(BOOL alphaTest,
                GX2CompareFunction func,
                float ref);

void
GX2InitAlphaTestReg(GX2AlphaTestReg *reg,
                    BOOL alphaTest,
                    GX2CompareFunction func,
                    float ref);

void
GX2GetAlphaTestReg(const GX2AlphaTestReg *reg,
                   be_val<BOOL> *alphaTest,
                   be_val<GX2CompareFunction> *func,
                   be_val<float> *ref);

void
GX2SetAlphaTestReg(GX2AlphaTestReg *reg);

void
GX2SetAlphaToMask(BOOL alphaToMask,
                  GX2AlphaToMaskMode mode);

void
GX2InitAlphaToMaskReg(GX2AlphaToMaskReg *reg,
                      BOOL alphaToMask,
                      GX2AlphaToMaskMode mode);

void
GX2GetAlphaToMaskReg(const GX2AlphaToMaskReg *reg,
                     be_val<BOOL> *alphaToMask,
                     be_val<GX2AlphaToMaskMode> *mode);

void
GX2SetAlphaToMaskReg(GX2AlphaToMaskReg *reg);

void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha);

void
GX2InitBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                             float red,
                             float green,
                             float blue,
                             float alpha);

void
GX2GetBlendConstantColorReg(GX2BlendConstantColorReg *reg,
                            be_val<float> *red,
                            be_val<float> *green,
                            be_val<float> *blue,
                            be_val<float> *alpha);

void
GX2SetBlendConstantColorReg(GX2BlendConstantColorReg *reg);

void
GX2SetBlendControl(GX2RenderTarget target,
                   GX2BlendMode colorSrcBlend,
                   GX2BlendMode colorDstBlend,
                   GX2BlendCombineMode colorCombine,
                   BOOL useAlphaBlend,
                   GX2BlendMode alphaSrcBlend,
                   GX2BlendMode alphaDstBlend,
                   GX2BlendCombineMode alphaCombine);

void
GX2InitBlendControlReg(GX2BlendControlReg *reg,
                       GX2RenderTarget target,
                       GX2BlendMode colorSrcBlend,
                       GX2BlendMode colorDstBlend,
                       GX2BlendCombineMode colorCombine,
                       BOOL useAlphaBlend,
                       GX2BlendMode alphaSrcBlend,
                       GX2BlendMode alphaDstBlend,
                       GX2BlendCombineMode alphaCombine);

void
GX2GetBlendControlReg(GX2BlendControlReg *reg,
                      be_val<GX2RenderTarget> *target,
                      be_val<GX2BlendMode> *colorSrcBlend,
                      be_val<GX2BlendMode> *colorDstBlend,
                      be_val<GX2BlendCombineMode> *colorCombine,
                      be_val<BOOL> *useAlphaBlend,
                      be_val<GX2BlendMode> *alphaSrcBlend,
                      be_val<GX2BlendMode> *alphaDstBlend,
                      be_val<GX2BlendCombineMode> *alphaCombine);

void
GX2SetBlendControlReg(GX2BlendControlReg *reg);

void
GX2SetColorControl(GX2LogicOp rop3,
                   uint8_t targetBlendEnable,
                   BOOL multiWriteEnable,
                   BOOL colorWriteEnable);

void
GX2InitColorControlReg(GX2ColorControlReg *reg,
                       GX2LogicOp rop3,
                       uint8_t targetBlendEnable,
                       BOOL multiWriteEnable,
                       BOOL colorWriteEnable);

void
GX2GetColorControlReg(GX2ColorControlReg *reg,
                      be_val<GX2LogicOp> *rop3,
                      be_val<uint8_t> *targetBlendEnable,
                      be_val<BOOL> *multiWriteEnable,
                      be_val<BOOL> *colorWriteEnable);

void
GX2SetColorControlReg(GX2ColorControlReg *reg);

void
GX2SetDepthOnlyControl(BOOL depthTest,
                       BOOL depthWrite,
                       GX2CompareFunction depthCompare);

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
                          GX2StencilFunction backStencilFail);

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
                              GX2StencilFunction backStencilFail);

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
                             be_val<GX2StencilFunction> *backStencilFail);

void
GX2SetDepthStencilControlReg(GX2DepthStencilControlReg *reg);

void
GX2SetStencilMask(uint8_t frontMask, uint8_t frontWriteMask, uint8_t frontRef,
                  uint8_t backMask, uint8_t backWriteMask, uint8_t backRef);

void
GX2InitStencilMaskReg(GX2StencilMaskReg *reg,
                      uint8_t frontMask, uint8_t frontWriteMask, uint8_t frontRef,
                      uint8_t backMask, uint8_t backWriteMask, uint8_t backRef);

void
GX2GetStencilMaskReg(GX2StencilMaskReg *reg,
                     uint8_t *frontMask, uint8_t *frontWriteMask, uint8_t *frontRef,
                     uint8_t *backMask, uint8_t *backWriteMask, uint8_t *backRef);

void
GX2SetStencilMaskReg(GX2StencilMaskReg *reg);

void
GX2SetLineWidth(float width);

void
GX2InitLineWidthReg(GX2LineWidthReg *reg,
                    float width);

void
GX2GetLineWidthReg(GX2LineWidthReg *reg,
                   be_val<float> *width);

void
GX2SetLineWidthReg(GX2LineWidthReg *reg);

void
GX2SetPointSize(float width,
                float height);

void
GX2InitPointSizeReg(GX2PointSizeReg *reg,
                    float width,
                    float height);

void
GX2GetPointSizeReg(GX2PointSizeReg *reg,
                   be_val<float> *width,
                   be_val<float> *height);

void
GX2SetPointSizeReg(GX2PointSizeReg *reg);

void
GX2SetPointLimits(float min,
                  float max);

void
GX2InitPointLimitsReg(GX2PointLimitsReg *reg,
                      float min,
                      float max);

void
GX2GetPointLimitsReg(GX2PointLimitsReg *reg,
                     be_val<float> *min,
                     be_val<float> *max);

void
GX2SetPointLimitsReg(GX2PointLimitsReg *reg);

void
GX2SetCullOnlyControl(GX2FrontFace frontFace,
                      BOOL cullFront,
                      BOOL cullBack);

void
GX2SetPolygonControl(GX2FrontFace frontFace,
                     BOOL cullFront,
                     BOOL cullBack,
                     BOOL polyMode,
                     GX2PolygonMode polyModeFront,
                     GX2PolygonMode polyModeBack,
                     BOOL polyOffsetFrontEnable,
                     BOOL polyOffsetBackEnable,
                     BOOL polyOffsetParaEnable);

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
                         BOOL polyOffsetParaEnable);

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
                        be_val<BOOL> *polyOffsetParaEnable);

void
GX2SetPolygonControlReg(GX2PolygonControlReg *reg);

void
GX2SetPolygonOffset(float frontOffset,
                    float frontScale,
                    float backOffset,
                    float backScale,
                    float clamp);

void
GX2InitPolygonOffsetReg(GX2PolygonOffsetReg *reg,
                        float frontOffset,
                        float frontScale,
                        float backOffset,
                        float backScale,
                        float clamp);

void
GX2GetPolygonOffsetReg(GX2PolygonOffsetReg *reg,
                       be_val<float> *frontOffset,
                       be_val<float> *frontScale,
                       be_val<float> *backOffset,
                       be_val<float> *backScale,
                       be_val<float> *clamp);

void
GX2SetPolygonOffsetReg(GX2PolygonOffsetReg *reg);

void
GX2SetScissor(uint32_t x,
              uint32_t y,
              uint32_t width,
              uint32_t height);

void
GX2InitScissorReg(GX2ScissorReg *reg,
                  uint32_t x,
                  uint32_t y,
                  uint32_t width,
                  uint32_t height);

void
GX2GetScissorReg(GX2ScissorReg *reg,
                 be_val<uint32_t> *x,
                 be_val<uint32_t> *y,
                 be_val<uint32_t> *width,
                 be_val<uint32_t> *height);

void
GX2SetScissorReg(GX2ScissorReg *reg);

void
GX2SetTargetChannelMasks(GX2ChannelMask mask0,
                         GX2ChannelMask mask1,
                         GX2ChannelMask mask2,
                         GX2ChannelMask mask3,
                         GX2ChannelMask mask4,
                         GX2ChannelMask mask5,
                         GX2ChannelMask mask6,
                         GX2ChannelMask mask7);

void
GX2InitTargetChannelMasksReg(GX2TargetChannelMaskReg *reg,
                             GX2ChannelMask mask0,
                             GX2ChannelMask mask1,
                             GX2ChannelMask mask2,
                             GX2ChannelMask mask3,
                             GX2ChannelMask mask4,
                             GX2ChannelMask mask5,
                             GX2ChannelMask mask6,
                             GX2ChannelMask mask7);

void
GX2GetTargetChannelMasksReg(GX2TargetChannelMaskReg *reg,
                            be_val<GX2ChannelMask> *mask0,
                            be_val<GX2ChannelMask> *mask1,
                            be_val<GX2ChannelMask> *mask2,
                            be_val<GX2ChannelMask> *mask3,
                            be_val<GX2ChannelMask> *mask4,
                            be_val<GX2ChannelMask> *mask5,
                            be_val<GX2ChannelMask> *mask6,
                            be_val<GX2ChannelMask> *mask7);

void
GX2SetTargetChannelMasksReg(GX2TargetChannelMaskReg *reg);

void
GX2SetViewport(float x,
               float y,
               float width,
               float height,
               float nearZ,
               float farZ);

void
GX2InitViewportReg(GX2ViewportReg *reg,
                   float x,
                   float y,
                   float width,
                   float height,
                   float nearZ,
                   float farZ);

void
GX2GetViewportReg(GX2ViewportReg *reg,
                  be_val<float> *x,
                  be_val<float> *y,
                  be_val<float> *width,
                  be_val<float> *height,
                  be_val<float> *nearZ,
                  be_val<float> *farZ);

void
GX2SetViewportReg(GX2ViewportReg *reg);

void
GX2SetRasterizerClipControl(BOOL rasteriser,
                            BOOL zclipNear);

void
GX2SetRasterizerClipControlEx(BOOL rasteriser,
                              BOOL zclipNear,
                              BOOL halfZ);

void
GX2SetRasterizerClipControlHalfZ(BOOL rasteriser,
                                 BOOL zclipNear,
                                 BOOL halfZ);

} // namespace gx2
