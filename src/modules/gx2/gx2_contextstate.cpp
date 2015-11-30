#include "gpu/pm4_writer.h"
#include "gx2.h"
#include "gx2_contextstate.h"
#include "gx2_draw.h"
#include "gx2_shaders.h"
#include "gx2_registers.h"
#include <utility>

static GX2ContextState *
gActiveContext = nullptr;

static std::pair<uint32_t, uint32_t> ConfigRegisterRange[] =
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

static std::pair<uint32_t, uint32_t> ContextRegisterRange[] =
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

static std::pair<uint32_t, uint32_t> AluConstRange[] =
{
   { 0, 0x800 },
};

static std::pair<uint32_t, uint32_t> LoopConstRange[] =
{
   { 0, 0x60 },
};

static std::pair<uint32_t, uint32_t> ResourceRange[] =
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

static std::pair<uint32_t, uint32_t> SamplerRange[] =
{
   { 0, 0x36 },
   { 0x36, 0x36 },
   { 0x6C, 0x36 },
};

void
GX2SetupContextState(GX2ContextState *state)
{
   GX2SetupContextStateEx(state, TRUE);
}

static void
_GX2LoadState(GX2ContextState *state)
{
   pm4::write(pm4::LoadConfigReg { state->shadowState.config, gsl::as_span(ConfigRegisterRange) });
   pm4::write(pm4::LoadContextReg { state->shadowState.context, gsl::as_span(ContextRegisterRange) });
   pm4::write(pm4::LoadAluConst { state->shadowState.alu, gsl::as_span(AluConstRange) });
   pm4::write(pm4::LoadLoopConst { state->shadowState.loop, gsl::as_span(LoopConstRange) });
   pm4::write(pm4::LoadResource { state->shadowState.resource, gsl::as_span(ResourceRange) });
   pm4::write(pm4::LoadSampler { state->shadowState.sampler, gsl::as_span(SamplerRange) });
}

void
GX2SetupContextStateEx(GX2ContextState *state, BOOL unk1)
{
   // Create our internal shadow display list
   memset(&state->shadowState, 0, sizeof(state->shadowState));
   GX2BeginDisplayList(state->shadowDisplayList, GX2ContextState::MaxDisplayListSize * 4);
   _GX2LoadState(state);
   state->shadowDisplayListSize = GX2EndDisplayList(state->shadowDisplayList);

   // Set to active state
   GX2SetContextState(state);

   // Initialise default state
   GX2SetDefaultState();
}

void
GX2GetContextStateDisplayList(GX2ContextState *state, be_ptr<void> *outDisplayList, be_val<uint32_t> *outSize)
{
   if (outDisplayList) {
      *outDisplayList = state->shadowDisplayList;
   }

   if (outSize) {
      *outSize = state->shadowDisplayListSize;
   }
}

void
GX2SetContextState(GX2ContextState *state)
{
   gActiveContext = state;

   if (state) {
      if (GX2GetDisplayListWriteStatus()) {
         GX2CopyDisplayList(state->shadowDisplayList, state->shadowDisplayListSize);
      } else {
         GX2CallDisplayList(state->shadowDisplayList, state->shadowDisplayListSize);
      }
   }

   pm4::write(pm4::DecafSetContextState {
      state ? reinterpret_cast<void *>(&state->shadowState) : nullptr
   });
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
      GX2SetBlendControl(static_cast<GX2RenderTarget::Value>(i),  // target
                         GX2BlendMode::SrcAlpha,      // colorSrcBlend
                         GX2BlendMode::InvSrcAlpha,   // colorDstBlend
                         GX2BlendCombineMode::Add,    // colorCombine
                         TRUE,                        // useAlphaBlend
                         GX2BlendMode::SrcAlpha,      // alphaSrcBlend
                         GX2BlendMode::InvSrcAlpha,   // alphaDstBlend
                         GX2BlendCombineMode::Add);   // alphaCombine
   }

   GX2SetBlendConstantColor(0.0f, 0.0f, 0.0f, 0.0f); // RGBA

   // GX2SetStreamOutEnable(0); VGT_STRMOUT_EN

   // GX2SetRasterizerClipControl(1, 1); PA_CL_CLIP_CNTL

   // GX2SetTessellation(0, 0x84, 9); 0x285 VGT_HOS_CNTL, 0x289 VGT_GROUP_PRIM_TYPE, 0x28a, 0x28b, 0x28c, 0x28e, 0x28d, 0x28f

   // GX2SetMaxTessellationLevel(1.0f); 0x286

   // GX2SetMinTessellationLevel(1.0f); 0x287

   // Set 0x343 DB_RENDER_CONTROL to 0
}
