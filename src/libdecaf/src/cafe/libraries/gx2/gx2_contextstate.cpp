#include "gx2.h"
#include "gx2_contextstate.h"
#include "gx2_draw.h"
#include "gx2_internal_cbpool.h"
#include "gx2_registers.h"
#include "gx2_shaders.h"
#include "gx2_state.h"
#include "gx2_tessellation.h"

#include <libgpu/latte/latte_pm4_commands.h>
#include <utility>

using namespace latte::pm4;

namespace cafe::gx2
{

static std::pair<uint32_t, uint32_t>
ConfigRegisterRange[] =
{
   { 0x300, 6 },
   { 0x900, 0x48 },
   { 0x980, 0x48 },
   { 0xA00, 0x48 },
   { 0x310, 0xC },
   { 0x542, 1 },
   { 0x235, 1 },
   { 0x232, 2 },
   { 0x23A, 1 },
   { 0x256, 1 },
   { 0x60C, 1 },
   { 0x5C5, 1 },
   { 0x2C8, 1 },
   { 0x363, 1 },
   { 0x404, 2 },
};

static std::pair<uint32_t, uint32_t>
ContextRegisterRange[] =
{
   { 0, 2 },
   { 3, 3 },
   { 0xA, 4 },
   { 0x10, 0x38 },
   { 0x50, 0x34 },
   { 0x8E, 4 },
   { 0x94, 0x40 },
   { 0x100, 9 },
   { 0x10C, 3 },
   { 0x10F, 0x60 },
   { 0x185, 0xA },
   { 0x191, 0x27 },
   { 0x1E0, 9 },
   { 0x200, 1 },
   { 0x202, 7 },
   { 0xE0, 0x20 },
   { 0x210, 0x29 },
   { 0x250, 0x34 },
   { 0x290, 1 },
   { 0x292, 2 },
   { 0x2A1, 1 },
   { 0x2A5, 1 },
   { 0x2A8, 2 },
   { 0x2AC, 3 },
   { 0x2CA, 1 },
   { 0x2CC, 1 },
   { 0x2CE, 1 },
   { 0x300, 9 },
   { 0x30C, 1 },
   { 0x312, 1 },
   { 0x316, 2 },
   { 0x343, 2 },
   { 0x349, 3 },
   { 0x34C, 2 },
   { 0x351, 1 },
   { 0x37E, 6 },
   { 0x2B4, 3 },
   { 0x2B8, 3 },
   { 0x2BC, 3 },
   { 0x2C0, 3 },
   { 0x2C8, 1 },
   { 0x29B, 1 },
   { 0x8C, 1 },
   { 0xD5, 1 },
   { 0x284, 0xC },
};

static std::pair<uint32_t, uint32_t>
AluConstRange[] =
{
   { 0, 0x800 },
};

static std::pair<uint32_t, uint32_t>
LoopConstRange[] =
{
   { 0, 0x60 },
};

static std::pair<uint32_t, uint32_t>
ResourceRange[] =
{
   { 0, 0x70 },
   { 0x380, 0x70 },
   { 0x460, 0x70 },
   { 0x7E0, 0x70 },
   { 0x8B9, 7 },
   { 0x8C0, 0x70 },
   { 0x930, 0x70 },
   { 0xCB0, 0x70 },
   { 0xD89, 7 },
};

static std::pair<uint32_t, uint32_t>
SamplerRange[] =
{
   { 0, 0x36 },
   { 0x36, 0x36 },
   { 0x6C, 0x36 },
};

static std::pair<uint32_t, uint32_t>
EmptyRange[] =
{
   { 0, 0 },
};

static auto
EmptyRangeSpan = gsl::make_span(EmptyRange);

static void
loadState(virt_ptr<GX2ContextState> state,
          bool skipLoad)
{
   internal::enableStateShadowing();

   internal::writePM4(LoadConfigReg {
      virt_addrof(state->shadowState.config),
      skipLoad ? EmptyRangeSpan : gsl::make_span(ConfigRegisterRange)
   });

   internal::writePM4(LoadContextReg {
      virt_addrof(state->shadowState.context),
      skipLoad ? EmptyRangeSpan : gsl::make_span(ContextRegisterRange)
   });

   internal::writePM4(LoadAluConst {
      virt_addrof(state->shadowState.alu),
      skipLoad ? EmptyRangeSpan : gsl::make_span(AluConstRange)
   });

   internal::writePM4(LoadLoopConst {
      virt_addrof(state->shadowState.loop),
      skipLoad ? EmptyRangeSpan : gsl::make_span(LoopConstRange)
   });

   internal::writePM4(latte::pm4::LoadResource {
      virt_addrof(state->shadowState.resource),
      skipLoad ? EmptyRangeSpan : gsl::make_span(ResourceRange)
   });

   internal::writePM4(LoadSampler {
      virt_addrof(state->shadowState.sampler),
      skipLoad ? EmptyRangeSpan : gsl::make_span(SamplerRange)
   });
}

void
GX2SetupContextStateEx(virt_ptr<GX2ContextState> state,
                       GX2ContextStateFlags flags)
{
   // Create our internal shadow display list
   std::memset(state.getRawPointer(), 0, sizeof(GX2ContextState));
   state->profileMode = (flags & GX2ContextStateFlags::ProfileMode) ? TRUE : FALSE;

   // Clear load state
   loadState(state, true);

   // Initialise default state
   internal::initialiseRegisters();
   GX2SetDefaultState();

   // Setup shadow display list
   if (!(flags & GX2ContextStateFlags::NoShadowDisplayList)) {
      GX2BeginDisplayList(virt_addrof(state->shadowDisplayList),
                          GX2ContextState::MaxDisplayListSize * 4);
      loadState(state, false);
      state->shadowDisplayListSize = GX2EndDisplayList(virt_addrof(state->shadowDisplayList));
   }
}

void
GX2GetContextStateDisplayList(virt_ptr<GX2ContextState> state,
                              virt_ptr<virt_ptr<void>> outDisplayList,
                              virt_ptr<uint32_t> outSize)
{
   if (outDisplayList) {
      *outDisplayList = virt_addrof(state->shadowDisplayList);
   }

   if (outSize) {
      *outSize = state->shadowDisplayListSize;
   }
}

void
GX2SetContextState(virt_ptr<GX2ContextState> state)
{
   if (state) {
      if (!state->shadowDisplayListSize) {
         loadState(state, false);
      } else {
         GX2CallDisplayList(virt_addrof(state->shadowDisplayList),
                            state->shadowDisplayListSize);
      }
   } else {
      internal::disableStateShadowing();
   }
}

void
GX2SetDefaultState()
{
   GX2SetShaderModeEx(GX2ShaderMode::UniformRegister, // mode
                      48,     // numVsGpr
                      64,     // numVsStackEntries
                      0,      // numGsGpr
                      0,      // numGsStackEntries
                      200,    // numPsGpr
                      192);   // numPsStackEntries

   GX2SetDepthStencilControl(TRUE,                          // depthTest
                             TRUE,                          // depthWrite
                             GX2CompareFunction::Less,      // depthCompare
                             FALSE,                         // stencilTest
                             FALSE,                         // backfaceStencil
                             GX2CompareFunction::Always,    // frontStencilFunc
                             GX2StencilFunction::Replace,   // frontStencilZPass
                             GX2StencilFunction::Replace,   // frontStencilZFail
                             GX2StencilFunction::Replace,   // frontStencilFail
                             GX2CompareFunction::Always,    // backStencilFunc
                             GX2StencilFunction::Replace,   // backStencilZPass
                             GX2StencilFunction::Replace,   // backStencilZFail
                             GX2StencilFunction::Replace);  // backStencilFail

   GX2SetPolygonControl(GX2FrontFace::CounterClockwise,  // frontFace
                        FALSE,                           // cullFront
                        FALSE,                           // cullBack
                        FALSE,                           // polyMode
                        GX2PolygonMode::Triangle,        // polyModeFront
                        GX2PolygonMode::Triangle,        // polyModeBack
                        FALSE,                           // polyOffsetFrontEnable
                        FALSE,                           // polyOffsetBackEnable
                        FALSE);                          // polyOffsetParaEnable

   GX2SetStencilMask(0xff,    // frontMask
                     0xff,    // frontWriteMask
                     1,       // frontRef
                     0xff,    // backMask
                     0xff,    // backWriteMask
                     1);      // backRef

   GX2SetPolygonOffset(0.0f,  // frontOffset
                       0.0f,  // frontScale
                       0.0f,  // backOffset
                       0.0f,  // backScale
                       0.0f); // clamp

   GX2SetPointSize(1.0f,      // width
                   1.0f);     // height

   GX2SetPointLimits(1.0f,    // min
                     1.0f);   // max

   GX2SetLineWidth(1.0f);

   GX2SetPrimitiveRestartIndex(-1);

   GX2SetAlphaTest(FALSE,                    // alphaTest
                   GX2CompareFunction::Less, // func
                   0.0f);                    // ref

   GX2SetAlphaToMask(FALSE,                              // enable
                     GX2AlphaToMaskMode::NonDithered);   // mode

   GX2SetTargetChannelMasks(GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA,
                            GX2ChannelMask::RGBA);

   GX2SetColorControl(GX2LogicOp::Copy,   // rop3
                      0,                  // targetBlendEnable
                      FALSE,              // multiWriteEnable
                      TRUE);              // colorWriteEnable

   for (auto i = 0u; i < 8u; ++i) {
      GX2SetBlendControl(static_cast<GX2RenderTarget>(i),  // target
                         GX2BlendMode::SrcAlpha,      // colorSrcBlend
                         GX2BlendMode::InvSrcAlpha,   // colorDstBlend
                         GX2BlendCombineMode::Add,    // colorCombine
                         TRUE,                        // useAlphaBlend
                         GX2BlendMode::SrcAlpha,      // alphaSrcBlend
                         GX2BlendMode::InvSrcAlpha,   // alphaDstBlend
                         GX2BlendCombineMode::Add);   // alphaCombine
   }

   GX2SetBlendConstantColor(0.0f, 0.0f, 0.0f, 0.0f); // RGBA

   GX2SetStreamOutEnable(0);

   GX2SetRasterizerClipControl(TRUE,   // rasteriser
                               TRUE);  // zclipEnable

   // TODO: Figure out what GX2PrimitiveMode 0x84 is
   GX2SetTessellation(GX2TessellationMode::Discrete, static_cast<GX2PrimitiveMode>(0x84), GX2IndexType::U32);

   GX2SetMaxTessellationLevel(1.0f);
   GX2SetMinTessellationLevel(1.0f);

   internal::writePM4(SetContextReg { latte::Register::DB_RENDER_CONTROL, 0 });
}

namespace internal
{

void
initialiseRegisters()
{
   std::array<uint32_t, 24> zeroes;
   zeroes.fill(0);

   uint32_t values28030_28034[] = {
      latte::PA_SC_SCREEN_SCISSOR_TL::get(0).value,
      latte::PA_SC_SCREEN_SCISSOR_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192).value
   };

   internal::writePM4(SetContextRegs {
      latte::Register::PA_SC_SCREEN_SCISSOR_TL,
      gsl::make_span(values28030_28034)
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_SC_LINE_CNTL,
      latte::PA_SC_LINE_CNTL::get(0)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_SU_VTX_CNTL,
      latte::PA_SU_VTX_CNTL::get(0)
         .PIX_CENTER(latte::PA_SU_VTX_CNTL_PIX_CENTER::OGL)
         .ROUND_MODE(latte::PA_SU_VTX_CNTL_ROUND_MODE::TRUNCATE)
         .QUANT_MODE(latte::PA_SU_VTX_CNTL_QUANT_MODE::QUANT_1_256TH)
         .value
   });

   // PA_CL_POINT_X_RAD, PA_CL_POINT_Y_RAD, PA_CL_POINT_POINT_SIZE, PA_CL_POINT_POINT_CULL_RAD
   internal::writePM4(SetContextRegs {
      latte::Register::PA_CL_POINT_X_RAD,
      gsl::make_span(zeroes.data(), 4)
   });

   // PA_CL_UCP_0_X ... PA_CL_UCP_5_W
   internal::writePM4(SetContextRegs {
      latte::Register::PA_CL_UCP_0_X,
      gsl::make_span(zeroes.data(), 24)
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_CL_VTE_CNTL,
      latte::PA_CL_VTE_CNTL::get(0)
         .VPORT_X_SCALE_ENA(true)
         .VPORT_X_OFFSET_ENA(true)
         .VPORT_Y_SCALE_ENA(true)
         .VPORT_Y_OFFSET_ENA(true)
         .VPORT_Z_SCALE_ENA(true)
         .VPORT_Z_OFFSET_ENA(true)
         .VTX_W0_FMT(true)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_CL_NANINF_CNTL,
      latte::PA_CL_NANINF_CNTL::get(0)
         .value
   });

   uint32_t values28200_28208[] = {
      0,
      latte::PA_SC_WINDOW_SCISSOR_TL::get(0)
         .WINDOW_OFFSET_DISABLE(true)
         .value,
      latte::PA_SC_WINDOW_SCISSOR_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192)
         .value,
   };

   internal::writePM4(SetContextRegs {
      latte::Register::PA_SC_WINDOW_OFFSET,
      gsl::make_span(values28200_28208)
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_SC_LINE_STIPPLE,
      latte::PA_SC_LINE_STIPPLE::get(0)
      .value
   });

   uint32_t values28A0C_28A10[] = {
      latte::PA_SC_MPASS_PS_CNTL::get(0)
         .value,
      latte::PA_SC_MODE_CNTL::get(0)
         .MSAA_ENABLE(true)
         .FORCE_EOV_CNTDWN_ENABLE(true)
         .FORCE_EOV_REZ_ENABLE(true)
         .value
   };

   internal::writePM4(SetContextRegs {
      latte::Register::PA_SC_LINE_STIPPLE,
      gsl::make_span(values28A0C_28A10)
   });

   uint32_t values28250_28254[] = {
      latte::PA_SC_VPORT_SCISSOR_0_TL::get(0)
         .WINDOW_OFFSET_DISABLE(true)
         .value,
      latte::PA_SC_VPORT_SCISSOR_0_BR::get(0)
         .BR_X(8192)
         .BR_Y(8192)
         .value,
   };

   internal::writePM4(SetContextRegs {
      latte::Register::PA_SC_VPORT_SCISSOR_0_TL,
      gsl::make_span(values28250_28254)
   });

   // TODO: Register 0x8B24 unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x8B24),
      0xFF3FFF
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_SC_CLIPRECT_RULE,
      latte::PA_SC_CLIPRECT_RULE::get(0)
         .CLIP_RULE(0xFFFF)
         .value
   });

   internal::writePM4(SetConfigReg {
      latte::Register::VGT_GS_VERTEX_REUSE,
      latte::VGT_GS_VERTEX_REUSE::get(0)
         .VERT_REUSE(16)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_OUTPUT_PATH_CNTL,
      latte::VGT_OUTPUT_PATH_CNTL::get(0)
         .PATH_SELECT(latte::VGT_OUTPUT_PATH_SELECT::TESS_EN)
         .value
   });

   // TODO: This is an unknown value 16 * 0xb14(r31) * 0xb18(r31)
   internal::writePM4(SetConfigReg {
      latte::Register::VGT_ES_PER_GS,
      latte::VGT_ES_PER_GS::get(0)
         .ES_PER_GS(16 * 1 * 1)
         .value
   });

   internal::writePM4(SetConfigReg {
      latte::Register::VGT_GS_PER_ES,
      latte::VGT_GS_PER_ES::get(0)
         .GS_PER_ES(256)
         .value
   });

   internal::writePM4(SetConfigReg {
      latte::Register::VGT_GS_PER_VS,
      latte::VGT_GS_PER_VS::get(0)
         .GS_PER_VS(4)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_INDX_OFFSET,
      latte::VGT_INDX_OFFSET::get(0)
         .INDX_OFFSET(0)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_REUSE_OFF,
      latte::VGT_REUSE_OFF::get(0)
         .REUSE_OFF(false)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_MULTI_PRIM_IB_RESET_EN,
      latte::VGT_MULTI_PRIM_IB_RESET_EN::get(0)
         .RESET_EN(true)
         .value
   });

   uint32_t values28C58_28C5C[] = {
      latte::VGT_VERTEX_REUSE_BLOCK_CNTL::get(0)
         .VTX_REUSE_DEPTH(14)
         .value,
      latte::VGT_OUT_DEALLOC_CNTL::get(0)
         .DEALLOC_DIST(16)
         .value,
   };

   internal::writePM4(SetContextRegs {
      latte::Register::VGT_VERTEX_REUSE_BLOCK_CNTL,
      gsl::make_span(values28C58_28C5C)
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_HOS_REUSE_DEPTH,
      latte::VGT_HOS_REUSE_DEPTH::get(0)
         .REUSE_DEPTH(16)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_STRMOUT_DRAW_OPAQUE_OFFSET,
      latte::VGT_STRMOUT_DRAW_OPAQUE_OFFSET::get(0)
         .OFFSET(0)
         .value
   });

   internal::writePM4(SetContextReg {
      latte::Register::VGT_VTX_CNT_EN,
      latte::VGT_VTX_CNT_EN::get(0)
         .VTX_CNT_EN(false)
         .value
   });

   uint32_t values28400_28404[] = {
      latte::VGT_MAX_VTX_INDX::get(0)
         .MAX_INDX(-1)
         .value,
      latte::VGT_MIN_VTX_INDX::get(0)
         .MIN_INDX(0)
         .value
   };

   internal::writePM4(SetContextRegs {
      latte::Register::VGT_MAX_VTX_INDX,
      gsl::make_span(values28400_28404)
   });

   internal::writePM4(SetConfigReg {
      latte::Register::TA_CNTL_AUX,
      latte::TA_CNTL_AUX::get(0)
         .UNK0(true)
         .SYNC_GRADIENT(true)
         .SYNC_WALKER(true)
         .SYNC_ALIGNER(true)
         .value
   });

   // TODO: Register 0x9714 unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x9714),
      1
   });

   // TODO: Register 0x8D8C unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x8D8C),
      0x4000
   });

   // SQ_ESTMP_RING_BASE ... SQ_REDUC_RING_SIZE
   internal::writePM4(SetConfigRegs {
      latte::Register::SQ_ESTMP_RING_BASE,
      gsl::make_span(zeroes.data(), 12)
   });

   // SQ_ESTMP_RING_ITEMSIZE ... SQ_REDUC_RING_ITEMSIZE
   internal::writePM4(SetContextRegs {
      latte::Register::SQ_ESTMP_RING_ITEMSIZE,
      gsl::make_span(zeroes.data(), 6)
   });

   internal::writePM4(SetControlConstant {
      latte::Register::SQ_VTX_START_INST_LOC,
      latte::SQ_VTX_START_INST_LOC::get(0)
         .OFFSET(0)
         .value
   });

   // SPI_FOG_CNTL ... SPI_FOG_FUNC_BIAS
   internal::writePM4(SetContextRegs {
      latte::Register::SPI_FOG_CNTL,
      gsl::make_span(zeroes.data(), 3)
   });

   internal::writePM4(SetContextReg {
      latte::Register::SPI_INTERP_CONTROL_0,
      latte::SPI_INTERP_CONTROL_0::get(0)
         .FLAT_SHADE_ENA(true)
         .PNT_SPRITE_ENA(false)
         .PNT_SPRITE_OVRD_X(latte::SPI_PNT_SPRITE_SEL::SEL_S)
         .PNT_SPRITE_OVRD_Y(latte::SPI_PNT_SPRITE_SEL::SEL_T)
         .PNT_SPRITE_OVRD_Z(latte::SPI_PNT_SPRITE_SEL::SEL_0)
         .PNT_SPRITE_OVRD_W(latte::SPI_PNT_SPRITE_SEL::SEL_1)
         .PNT_SPRITE_TOP_1(true)
         .value
   });

   internal::writePM4(SetConfigReg {
      latte::Register::SPI_CONFIG_CNTL_1,
      latte::SPI_CONFIG_CNTL_1::get(0)
         .value
   });

   // TODO: Register 0x286C8 unknown
   internal::writePM4(SetAllContextsReg {
      static_cast<latte::Register>(0x286C8),
      1
   });

   // TODO: Register 0x28354 unknown
   auto unkValue = 0u; // 0x143C(r31)

   if (unkValue > 0x5270) {
      internal::writePM4(SetContextReg {
         static_cast<latte::Register>(0x28354),
         0xFF
      });
   } else {
      internal::writePM4(SetContextReg {
         static_cast<latte::Register>(0x28354),
         0x1FF
      });
   }

   uint32_t values28D28_28D2C[] = {
      latte::DB_SRESULTS_COMPARE_STATE0::get(0)
         .value,
      latte::DB_SRESULTS_COMPARE_STATE1::get(0)
         .value
   };

   internal::writePM4(SetContextRegs {
      latte::Register::DB_SRESULTS_COMPARE_STATE0,
      gsl::make_span(values28D28_28D2C)
   });

   internal::writePM4(SetContextReg {
      latte::Register::DB_RENDER_OVERRIDE,
      latte::DB_RENDER_OVERRIDE::get(0)
         .value
   });

   // TODO: Register 0x9830 unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x9830),
      0
   });

   // TODO: Register 0x983C unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x983C),
      0x1000000
   });

   uint32_t values28C30_28C3C[] = {
      latte::CB_CLRCMP_CONTROL::get(0)
         .CLRCMP_FCN_SEL(latte::CB_CLRCMP_SEL::SRC)
         .value,
      latte::CB_CLRCMP_SRC::get(0)
         .CLRCMP_SRC(0)
         .value,
      latte::CB_CLRCMP_DST::get(0)
         .CLRCMP_DST(0)
         .value,
      latte::CB_CLRCMP_MSK::get(0)
         .CLRCMP_MSK(0xFFFFFFFF)
         .value
   };

   internal::writePM4(SetContextRegs {
      latte::Register::CB_CLRCMP_CONTROL,
      gsl::make_span(values28C30_28C3C)
   });

   // TODO: Register 0x9A1C unknown
   internal::writePM4(SetConfigReg {
      static_cast<latte::Register>(0x9A1C),
      0
   });

   internal::writePM4(SetContextReg {
      latte::Register::PA_SC_AA_MASK,
      latte::PA_SC_AA_MASK::get(0)
         .AA_MASK_ULC(0xFF)
         .AA_MASK_URC(0xFF)
         .AA_MASK_LLC(0xFF)
         .AA_MASK_LRC(0xFF)
         .value
   });

   // TODO: Register 0x28230 unknown
   internal::writePM4(SetContextReg {
      static_cast<latte::Register>(0x28230),
      0xAAAAAAAA
   });

   // TODO: GX2SetAAMode(0);
}

} // namespace internal

void
Library::registerContextStateSymbols()
{
   RegisterFunctionExport(GX2SetupContextStateEx);
   RegisterFunctionExport(GX2GetContextStateDisplayList);
   RegisterFunctionExport(GX2SetContextState);
   RegisterFunctionExport(GX2SetDefaultState);
}

} // namespace cafe::gx2
