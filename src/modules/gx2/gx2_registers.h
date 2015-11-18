#pragma once
#include "types.h"
#include "gpu/latte_registers.h"
#include "gx2_enum.h"
#include "utils/structsize.h"

struct GX2DepthBuffer;

#pragma pack(push, 1)

struct GX2AAMaskReg
{
   be_val<latte::PA_SC_AA_MASK> pa_sc_aa_mask;
};

struct GX2AlphaTestReg
{
   be_val<latte::SX_ALPHA_TEST_CONTROL> sx_alpha_test_control;
   be_val<latte::SX_ALPHA_REF> sx_alpha_ref;
};

struct GX2BlendControlReg
{
   be_val<GX2RenderTarget::Value> target;
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

struct GX2DepthStencilControlReg
{
   be_val<latte::DB_DEPTH_CONTROL> db_depth_control;
};
CHECK_SIZE(GX2DepthStencilControlReg, 4);
CHECK_OFFSET(GX2DepthStencilControlReg, 0, db_depth_control);

struct GX2LineWidthReg
{
   // be_val<latte::PA_SU_LINE_CNTL> pa_su_line_cntl;
};

struct GX2PointSizeReg
{
   // be_val<latte::PA_SU_POINT_SIZE> pa_su_point_size;
};

struct GX2PointLimitsReg
{
   // be_val<latte::PA_SU_POINT_MINMAX> pa_su_point_minmax;
};

struct GX2PolygonControlReg
{
   // be_val<latte::PA_SU_SC_MODE_CNTL> pa_su_sc_mode_cntl;
};

struct GX2PolygonOffsetReg
{
   // be_val<latte::PA_SU_POLY_OFFSET_FRONT_SCALE> pa_su_poly_offset_front_scale;
   // be_val<latte::PA_SU_POLY_OFFSET_FRONT_OFFSET> pa_su_poly_offset_front_offset;
   // be_val<latte::PA_SU_POLY_OFFSET_BACK_SCALE> pa_su_poly_offset_back_scale;
   // be_val<latte::PA_SU_POLY_OFFSET_BACK_OFFSET> pa_su_poly_offset_back_offset;

   // be_val<latte::PA_SU_POLY_OFFSET_CLAMP> pa_su_poly_offset_clamp;
};

struct GX2ScissorReg
{
   // be_val<latte::PA_SC_GENERIC_SCISSOR_TL> pa_sc_generic_scissor_tl;
   // be_val<latte::PA_SC_GENERIC_SCISSOR_BR> pa_sc_generic_scissor_br;
};

struct GX2TargetChannelMaskReg
{
   // be_val<latte::CB_TARGET_MASK> cb_target_mask;
};

struct GX2ViewportReg
{
   // be_val<latte::PA_CL_VPORT_XSCALE_N> pa_cl_vport_xscale;
   // be_val<latte::PA_CL_VPORT_XOFFSET_N> pa_cl_vport_xoffset;
   // be_val<latte::PA_CL_VPORT_YSCALE_N> pa_cl_vport_yscale;
   // be_val<latte::PA_CL_VPORT_YOFFSET_N> pa_cl_vport_yoffset;
   // be_val<latte::PA_CL_VPORT_ZSCALE_N> pa_cl_vport_zscale;
   // be_val<latte::PA_CL_VPORT_ZOFFSET_N> pa_cl_vport_zoffset;

   // be_val<latte::PA_CL_GB_VERT_CLIP_ADJ> pa_cl_gb_vert_clip_adj;
   // be_val<latte::PA_CL_GB_VERT_DISC_ADJ> pa_cl_gb_vert_disc_adj;
   // be_val<latte::PA_CL_GB_HORZ_CLIP_ADJ> pa_cl_gb_horz_clip_adj;
   // be_val<latte::PA_CL_GB_HORZ_DISC_ADJ> pa_cl_gb_horz_disc_adj;

   // be_val<latte::PA_SC_VPORT_ZMIN_N> pa_sc_vport_zmin;
   // be_val<latte::PA_SC_VPORT_ZMAX_N> pa_sc_vport_zmax;
};

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

void
GX2SetPrimitiveRestartIndex(uint32_t index);
