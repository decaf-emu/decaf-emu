#include "gx2.h"
#include "gx2_renderstate.h"

void
GX2SetDepthStencilControl(uint32_t unk1,
                          uint32_t unk2,
                          uint32_t unk3,
                          uint32_t unk4,
                          uint32_t unk5,
                          uint32_t unk6,
                          uint32_t unk7,
                          uint32_t unk8,
                          uint32_t unk9,
                          uint32_t unk10,
                          uint32_t unk11,
                          uint32_t unk12,
                          uint32_t unk13)
{
   // TODO: GX2SetDepthStencilControl
}

void
GX2SetStencilMask(uint32_t unk1,
                  uint32_t unk2,
                  uint32_t unk3,
                  uint32_t unk4,
                  uint32_t unk5,
                  uint32_t unk6)
{
   // TODO: GX2SetStencilMask
}

void
GX2SetPolygonControl(uint32_t unk1,
                     uint32_t unk2,
                     uint32_t unk3,
                     uint32_t unk4,
                     uint32_t unk5,
                     uint32_t unk6,
                     uint32_t unk7,
                     uint32_t unk8,
                     uint32_t unk9)
{
   // TODO: GX2SetPolygonControl
}

void
GX2SetColorControl(GX2LogicOp::Op logicOp,
                   uint32_t unk1,
                   uint32_t unk2,
                   uint32_t unk3)
{
   // TODO: GX2SetColorControl
}

void
GX2SetBlendControl(GX2RenderTarget::Target target,
                   GX2BlendMode::Mode colorSrcBlend,
                   GX2BlendMode::Mode colorDstBlend,
                   GX2BlendCombineMode::Mode colorCombine,
                   uint32_t unk1,
                   GX2BlendMode::Mode alphaSrcBlend,
                   GX2BlendMode::Mode alphaDstBlend,
                   GX2BlendCombineMode::Mode alphaCombine)
{
   // TODO: GX2SetBlendControl
}

void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha)
{
   // TODO: GX2SetBlendConstantColor
}

void
GX2SetAlphaTest(BOOL enabled,
                GX2CompareFunction::Func compare,
                float reference)
{
   // TODO: GX2SetAlphaTest
}

void
GX2SetTargetChannelMasks(GX2ChannelMask::Mask target0,
                         GX2ChannelMask::Mask target1,
                         GX2ChannelMask::Mask target2,
                         GX2ChannelMask::Mask target3,
                         GX2ChannelMask::Mask target4,
                         GX2ChannelMask::Mask target5,
                         GX2ChannelMask::Mask target6)
{
   // TODO: GX2SetTargetChannelMasks
}

void
GX2SetAlphaToMask(BOOL enabled,
                  GX2AlphaToMaskMode::Mode mode)
{
   // TODO: GX2SetAlphaToMask
}

void
GX2SetViewport(float x1,
               float y1,
               float x2,
               float y2,
               float zNear,
               float zFar)
{
   // TODO: GX2SetViewport - ensure this is not x,y,w,h
   assert(x1 == 0 && y1 == 0);
}

void
GX2SetScissor(uint32_t x1,
              uint32_t y1,
              uint32_t x2,
              uint32_t y2)
{
   // TODO: GX2SetScissor - ensure this is not x,y,w,h
   assert(x1 == 0 && y1 == 0);
}

void
GX2::registerRenderStateFunctions()
{
   RegisterKernelFunction(GX2SetDepthStencilControl);
   RegisterKernelFunction(GX2SetStencilMask);
   RegisterKernelFunction(GX2SetPolygonControl);
   RegisterKernelFunction(GX2SetColorControl);
   RegisterKernelFunction(GX2SetBlendControl);
   RegisterKernelFunction(GX2SetBlendConstantColor);
   RegisterKernelFunction(GX2SetAlphaTest);
   RegisterKernelFunction(GX2SetTargetChannelMasks);
   RegisterKernelFunction(GX2SetAlphaToMask);
   RegisterKernelFunction(GX2SetViewport);
   RegisterKernelFunction(GX2SetScissor);
}
