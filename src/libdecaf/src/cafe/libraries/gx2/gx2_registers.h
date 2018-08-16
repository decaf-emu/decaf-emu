#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>
#include <libgpu/latte/latte_registers.h>

namespace cafe::gx2
{

#pragma pack(push, 1)

struct GX2AAMaskReg
{
   be2_val<latte::PA_SC_AA_MASK> pa_sc_aa_mask;
};
CHECK_SIZE(GX2AAMaskReg, 4);
CHECK_OFFSET(GX2AAMaskReg, 0, pa_sc_aa_mask);

struct GX2AlphaTestReg
{
   be2_val<latte::SX_ALPHA_TEST_CONTROL> sx_alpha_test_control;
   be2_val<latte::SX_ALPHA_REF> sx_alpha_ref;
};
CHECK_SIZE(GX2AlphaTestReg, 8);
CHECK_OFFSET(GX2AlphaTestReg, 0, sx_alpha_test_control);
CHECK_OFFSET(GX2AlphaTestReg, 4, sx_alpha_ref);

struct GX2AlphaToMaskReg
{
   be2_val<latte::DB_ALPHA_TO_MASK> db_alpha_to_mask;
};
CHECK_SIZE(GX2AlphaToMaskReg, 4);
CHECK_OFFSET(GX2AlphaToMaskReg, 0, db_alpha_to_mask);

struct GX2BlendControlReg
{
   be2_val<GX2RenderTarget> target;
   be2_val<latte::CB_BLENDN_CONTROL> cb_blend_control;
};
CHECK_SIZE(GX2BlendControlReg, 8);
CHECK_OFFSET(GX2BlendControlReg, 0, target);
CHECK_OFFSET(GX2BlendControlReg, 4, cb_blend_control);

struct GX2BlendConstantColorReg
{
   be2_val<float> red;
   be2_val<float> green;
   be2_val<float> blue;
   be2_val<float> alpha;
};
CHECK_SIZE(GX2BlendConstantColorReg, 0x10);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x00, red);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x04, green);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x08, blue);
CHECK_OFFSET(GX2BlendConstantColorReg, 0x0c, alpha);

struct GX2ColorControlReg
{
   be2_val<latte::CB_COLOR_CONTROL> cb_color_control;
};
CHECK_SIZE(GX2ColorControlReg, 0x04);
CHECK_OFFSET(GX2ColorControlReg, 0x00, cb_color_control);

struct GX2DepthStencilControlReg
{
   be2_val<latte::DB_DEPTH_CONTROL> db_depth_control;
};
CHECK_SIZE(GX2DepthStencilControlReg, 4);
CHECK_OFFSET(GX2DepthStencilControlReg, 0, db_depth_control);

struct GX2StencilMaskReg
{
   be2_val<latte::DB_STENCILREFMASK> db_stencilrefmask;
   be2_val<latte::DB_STENCILREFMASK_BF> db_stencilrefmask_bf;
};
CHECK_SIZE(GX2StencilMaskReg, 8);
CHECK_OFFSET(GX2StencilMaskReg, 0, db_stencilrefmask);
CHECK_OFFSET(GX2StencilMaskReg, 4, db_stencilrefmask_bf);

struct GX2LineWidthReg
{
   be2_val<latte::PA_SU_LINE_CNTL> pa_su_line_cntl;
};
CHECK_SIZE(GX2LineWidthReg, 4);
CHECK_OFFSET(GX2LineWidthReg, 0, pa_su_line_cntl);

struct GX2PointSizeReg
{
   be2_val<latte::PA_SU_POINT_SIZE> pa_su_point_size;
};
CHECK_SIZE(GX2PointSizeReg, 4);
CHECK_OFFSET(GX2PointSizeReg, 0, pa_su_point_size);

struct GX2PointLimitsReg
{
   be2_val<latte::PA_SU_POINT_MINMAX> pa_su_point_minmax;
};
CHECK_SIZE(GX2PointLimitsReg, 4);
CHECK_OFFSET(GX2PointLimitsReg, 0, pa_su_point_minmax);

struct GX2PolygonControlReg
{
   be2_val<latte::PA_SU_SC_MODE_CNTL> pa_su_sc_mode_cntl;
};
CHECK_SIZE(GX2PolygonControlReg, 4);
CHECK_OFFSET(GX2PolygonControlReg, 0, pa_su_sc_mode_cntl);

struct GX2PolygonOffsetReg
{
   be2_val<latte::PA_SU_POLY_OFFSET_FRONT_SCALE> pa_su_poly_offset_front_scale;
   be2_val<latte::PA_SU_POLY_OFFSET_FRONT_OFFSET> pa_su_poly_offset_front_offset;
   be2_val<latte::PA_SU_POLY_OFFSET_BACK_SCALE> pa_su_poly_offset_back_scale;
   be2_val<latte::PA_SU_POLY_OFFSET_BACK_OFFSET> pa_su_poly_offset_back_offset;

   be2_val<latte::PA_SU_POLY_OFFSET_CLAMP> pa_su_poly_offset_clamp;
};
CHECK_SIZE(GX2PolygonOffsetReg, 0x14);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x00, pa_su_poly_offset_front_scale);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x04, pa_su_poly_offset_front_offset);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x08, pa_su_poly_offset_back_scale);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x0C, pa_su_poly_offset_back_offset);
CHECK_OFFSET(GX2PolygonOffsetReg, 0x10, pa_su_poly_offset_clamp);

struct GX2ScissorReg
{
   be2_val<latte::PA_SC_GENERIC_SCISSOR_TL> pa_sc_generic_scissor_tl;
   be2_val<latte::PA_SC_GENERIC_SCISSOR_BR> pa_sc_generic_scissor_br;
};
CHECK_SIZE(GX2ScissorReg, 0x08);
CHECK_OFFSET(GX2ScissorReg, 0x00, pa_sc_generic_scissor_tl);
CHECK_OFFSET(GX2ScissorReg, 0x04, pa_sc_generic_scissor_br);

struct GX2TargetChannelMaskReg
{
   be2_val<latte::CB_TARGET_MASK> cb_target_mask;
};
CHECK_SIZE(GX2TargetChannelMaskReg, 0x04);
CHECK_OFFSET(GX2TargetChannelMaskReg, 0x00, cb_target_mask);

struct GX2ViewportReg
{
   be2_val<latte::PA_CL_VPORT_XSCALE_N> pa_cl_vport_xscale;
   be2_val<latte::PA_CL_VPORT_XOFFSET_N> pa_cl_vport_xoffset;
   be2_val<latte::PA_CL_VPORT_YSCALE_N> pa_cl_vport_yscale;
   be2_val<latte::PA_CL_VPORT_YOFFSET_N> pa_cl_vport_yoffset;
   be2_val<latte::PA_CL_VPORT_ZSCALE_N> pa_cl_vport_zscale;
   be2_val<latte::PA_CL_VPORT_ZOFFSET_N> pa_cl_vport_zoffset;

   be2_val<latte::PA_CL_GB_VERT_CLIP_ADJ> pa_cl_gb_vert_clip_adj;
   be2_val<latte::PA_CL_GB_VERT_DISC_ADJ> pa_cl_gb_vert_disc_adj;
   be2_val<latte::PA_CL_GB_HORZ_CLIP_ADJ> pa_cl_gb_horz_clip_adj;
   be2_val<latte::PA_CL_GB_HORZ_DISC_ADJ> pa_cl_gb_horz_disc_adj;

   be2_val<latte::PA_SC_VPORT_ZMIN_N> pa_sc_vport_zmin;
   be2_val<latte::PA_SC_VPORT_ZMAX_N> pa_sc_vport_zmax;
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
GX2InitAAMaskReg(virt_ptr<GX2AAMaskReg> reg,
                 uint8_t upperLeft,
                 uint8_t upperRight,
                 uint8_t lowerLeft,
                 uint8_t lowerRight);

void
GX2GetAAMaskReg(virt_ptr<GX2AAMaskReg> reg,
                virt_ptr<uint8_t> upperLeft,
                virt_ptr<uint8_t> upperRight,
                virt_ptr<uint8_t> lowerLeft,
                virt_ptr<uint8_t> lowerRight);

void
GX2SetAAMaskReg(virt_ptr<GX2AAMaskReg> reg);

void
GX2SetAlphaTest(BOOL alphaTest,
                GX2CompareFunction func,
                float ref);

void
GX2InitAlphaTestReg(virt_ptr<GX2AlphaTestReg> reg,
                    BOOL alphaTest,
                    GX2CompareFunction func,
                    float ref);

void
GX2GetAlphaTestReg(const virt_ptr<GX2AlphaTestReg> reg,
                   virt_ptr<BOOL> alphaTest,
                   virt_ptr<GX2CompareFunction> func,
                   virt_ptr<float> ref);

void
GX2SetAlphaTestReg(virt_ptr<GX2AlphaTestReg> reg);

void
GX2SetAlphaToMask(BOOL alphaToMask,
                  GX2AlphaToMaskMode mode);

void
GX2InitAlphaToMaskReg(virt_ptr<GX2AlphaToMaskReg> reg,
                      BOOL alphaToMask,
                      GX2AlphaToMaskMode mode);

void
GX2GetAlphaToMaskReg(const virt_ptr<GX2AlphaToMaskReg> reg,
                     virt_ptr<BOOL> alphaToMask,
                     virt_ptr<GX2AlphaToMaskMode> mode);

void
GX2SetAlphaToMaskReg(virt_ptr<GX2AlphaToMaskReg> reg);

void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha);

void
GX2InitBlendConstantColorReg(virt_ptr<GX2BlendConstantColorReg> reg,
                             float red,
                             float green,
                             float blue,
                             float alpha);

void
GX2GetBlendConstantColorReg(virt_ptr<GX2BlendConstantColorReg> reg,
                            virt_ptr<float> red,
                            virt_ptr<float> green,
                            virt_ptr<float> blue,
                            virt_ptr<float> alpha);

void
GX2SetBlendConstantColorReg(virt_ptr<GX2BlendConstantColorReg> reg);

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
GX2InitBlendControlReg(virt_ptr<GX2BlendControlReg> reg,
                       GX2RenderTarget target,
                       GX2BlendMode colorSrcBlend,
                       GX2BlendMode colorDstBlend,
                       GX2BlendCombineMode colorCombine,
                       BOOL useAlphaBlend,
                       GX2BlendMode alphaSrcBlend,
                       GX2BlendMode alphaDstBlend,
                       GX2BlendCombineMode alphaCombine);

void
GX2GetBlendControlReg(virt_ptr<GX2BlendControlReg> reg,
                      virt_ptr<GX2RenderTarget> target,
                      virt_ptr<GX2BlendMode> colorSrcBlend,
                      virt_ptr<GX2BlendMode> colorDstBlend,
                      virt_ptr<GX2BlendCombineMode> colorCombine,
                      virt_ptr<BOOL> useAlphaBlend,
                      virt_ptr<GX2BlendMode> alphaSrcBlend,
                      virt_ptr<GX2BlendMode> alphaDstBlend,
                      virt_ptr<GX2BlendCombineMode> alphaCombine);

void
GX2SetBlendControlReg(virt_ptr<GX2BlendControlReg> reg);

void
GX2SetColorControl(GX2LogicOp rop3,
                   uint8_t targetBlendEnable,
                   BOOL multiWriteEnable,
                   BOOL colorWriteEnable);

void
GX2InitColorControlReg(virt_ptr<GX2ColorControlReg> reg,
                       GX2LogicOp rop3,
                       uint8_t targetBlendEnable,
                       BOOL multiWriteEnable,
                       BOOL colorWriteEnable);

void
GX2GetColorControlReg(virt_ptr<GX2ColorControlReg> reg,
                      virt_ptr<GX2LogicOp> rop3,
                      virt_ptr<uint8_t> targetBlendEnable,
                      virt_ptr<BOOL> multiWriteEnable,
                      virt_ptr<BOOL> colorWriteEnable);

void
GX2SetColorControlReg(virt_ptr<GX2ColorControlReg> reg);

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
GX2InitDepthStencilControlReg(virt_ptr<GX2DepthStencilControlReg> reg,
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
GX2GetDepthStencilControlReg(virt_ptr<GX2DepthStencilControlReg> reg,
                             virt_ptr<BOOL> depthTest,
                             virt_ptr<BOOL> depthWrite,
                             virt_ptr<GX2CompareFunction> depthCompare,
                             virt_ptr<BOOL> stencilTest,
                             virt_ptr<BOOL> backfaceStencil,
                             virt_ptr<GX2CompareFunction> frontStencilFunc,
                             virt_ptr<GX2StencilFunction> frontStencilZPass,
                             virt_ptr<GX2StencilFunction> frontStencilZFail,
                             virt_ptr<GX2StencilFunction> frontStencilFail,
                             virt_ptr<GX2CompareFunction> backStencilFunc,
                             virt_ptr<GX2StencilFunction> backStencilZPass,
                             virt_ptr<GX2StencilFunction> backStencilZFail,
                             virt_ptr<GX2StencilFunction> backStencilFail);

void
GX2SetDepthStencilControlReg(virt_ptr<GX2DepthStencilControlReg> reg);

void
GX2SetStencilMask(uint8_t frontMask, uint8_t frontWriteMask, uint8_t frontRef,
                  uint8_t backMask, uint8_t backWriteMask, uint8_t backRef);

void
GX2InitStencilMaskReg(virt_ptr<GX2StencilMaskReg> reg,
                      uint8_t frontMask, uint8_t frontWriteMask, uint8_t frontRef,
                      uint8_t backMask, uint8_t backWriteMask, uint8_t backRef);

void
GX2GetStencilMaskReg(virt_ptr<GX2StencilMaskReg> reg,
                     virt_ptr<uint8_t> frontMask,
                     virt_ptr<uint8_t> frontWriteMask,
                     virt_ptr<uint8_t> frontRef,
                     virt_ptr<uint8_t> backMask,
                     virt_ptr<uint8_t> backWriteMask,
                     virt_ptr<uint8_t> backRef);

void
GX2SetStencilMaskReg(virt_ptr<GX2StencilMaskReg> reg);

void
GX2SetLineWidth(float width);

void
GX2InitLineWidthReg(virt_ptr<GX2LineWidthReg> reg,
                    float width);

void
GX2GetLineWidthReg(virt_ptr<GX2LineWidthReg> reg,
                   virt_ptr<float> width);

void
GX2SetLineWidthReg(virt_ptr<GX2LineWidthReg> reg);

void
GX2SetPointSize(float width,
                float height);

void
GX2InitPointSizeReg(virt_ptr<GX2PointSizeReg> reg,
                    float width,
                    float height);

void
GX2GetPointSizeReg(virt_ptr<GX2PointSizeReg> reg,
                   virt_ptr<float> width,
                   virt_ptr<float> height);

void
GX2SetPointSizeReg(virt_ptr<GX2PointSizeReg> reg);

void
GX2SetPointLimits(float min,
                  float max);

void
GX2InitPointLimitsReg(virt_ptr<GX2PointLimitsReg> reg,
                      float min,
                      float max);

void
GX2GetPointLimitsReg(virt_ptr<GX2PointLimitsReg> reg,
                     virt_ptr<float> min,
                     virt_ptr<float> max);

void
GX2SetPointLimitsReg(virt_ptr<GX2PointLimitsReg> reg);

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
GX2InitPolygonControlReg(virt_ptr<GX2PolygonControlReg> reg,
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
GX2GetPolygonControlReg(virt_ptr<GX2PolygonControlReg> reg,
                        virt_ptr<GX2FrontFace> frontFace,
                        virt_ptr<BOOL> cullFront,
                        virt_ptr<BOOL> cullBack,
                        virt_ptr<BOOL> polyMode,
                        virt_ptr<GX2PolygonMode> polyModeFront,
                        virt_ptr<GX2PolygonMode> polyModeBack,
                        virt_ptr<BOOL> polyOffsetFrontEnable,
                        virt_ptr<BOOL> polyOffsetBackEnable,
                        virt_ptr<BOOL> polyOffsetParaEnable);

void
GX2SetPolygonControlReg(virt_ptr<GX2PolygonControlReg> reg);

void
GX2SetPolygonOffset(float frontOffset,
                    float frontScale,
                    float backOffset,
                    float backScale,
                    float clamp);

void
GX2InitPolygonOffsetReg(virt_ptr<GX2PolygonOffsetReg> reg,
                        float frontOffset,
                        float frontScale,
                        float backOffset,
                        float backScale,
                        float clamp);

void
GX2GetPolygonOffsetReg(virt_ptr<GX2PolygonOffsetReg> reg,
                       virt_ptr<float> frontOffset,
                       virt_ptr<float> frontScale,
                       virt_ptr<float> backOffset,
                       virt_ptr<float> backScale,
                       virt_ptr<float> clamp);

void
GX2SetPolygonOffsetReg(virt_ptr<GX2PolygonOffsetReg> reg);

void
GX2SetScissor(uint32_t x,
              uint32_t y,
              uint32_t width,
              uint32_t height);

void
GX2InitScissorReg(virt_ptr<GX2ScissorReg> reg,
                  uint32_t x,
                  uint32_t y,
                  uint32_t width,
                  uint32_t height);

void
GX2GetScissorReg(virt_ptr<GX2ScissorReg> reg,
                 virt_ptr<uint32_t> x,
                 virt_ptr<uint32_t> y,
                 virt_ptr<uint32_t> width,
                 virt_ptr<uint32_t> height);

void
GX2SetScissorReg(virt_ptr<GX2ScissorReg> reg);

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
GX2InitTargetChannelMasksReg(virt_ptr<GX2TargetChannelMaskReg> reg,
                             GX2ChannelMask mask0,
                             GX2ChannelMask mask1,
                             GX2ChannelMask mask2,
                             GX2ChannelMask mask3,
                             GX2ChannelMask mask4,
                             GX2ChannelMask mask5,
                             GX2ChannelMask mask6,
                             GX2ChannelMask mask7);

void
GX2GetTargetChannelMasksReg(virt_ptr<GX2TargetChannelMaskReg> reg,
                            virt_ptr<GX2ChannelMask> mask0,
                            virt_ptr<GX2ChannelMask> mask1,
                            virt_ptr<GX2ChannelMask> mask2,
                            virt_ptr<GX2ChannelMask> mask3,
                            virt_ptr<GX2ChannelMask> mask4,
                            virt_ptr<GX2ChannelMask> mask5,
                            virt_ptr<GX2ChannelMask> mask6,
                            virt_ptr<GX2ChannelMask> mask7);

void
GX2SetTargetChannelMasksReg(virt_ptr<GX2TargetChannelMaskReg> reg);

void
GX2SetViewport(float x,
               float y,
               float width,
               float height,
               float nearZ,
               float farZ);

void
GX2InitViewportReg(virt_ptr<GX2ViewportReg> reg,
                   float x,
                   float y,
                   float width,
                   float height,
                   float nearZ,
                   float farZ);

void
GX2GetViewportReg(virt_ptr<GX2ViewportReg> reg,
                  virt_ptr<float> x,
                  virt_ptr<float> y,
                  virt_ptr<float> width,
                  virt_ptr<float> height,
                  virt_ptr<float> nearZ,
                  virt_ptr<float> farZ);

void
GX2SetViewportReg(virt_ptr<GX2ViewportReg> reg);

void
GX2SetRasterizerClipControl(BOOL rasteriser,
                            BOOL zclipEnable);

void
GX2SetRasterizerClipControlEx(BOOL rasteriser,
                              BOOL zclipEnable,
                              BOOL halfZ);

void
GX2SetRasterizerClipControlHalfZ(BOOL rasteriser,
                                 BOOL zclipEnable,
                                 BOOL halfZ);

} // namespace cafe::gx2
