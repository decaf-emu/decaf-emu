#include "../gx2.h"
#ifdef GX2_DX12

#include "../gx2_renderstate.h"
#include "dx12_state.h"

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
GX2SetStencilMask(uint8_t unk1,
   uint8_t unk2,
   uint8_t unk3,
   uint8_t unk4,
   uint8_t unk5,
   uint8_t unk6)
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
GX2SetViewport(float x,
   float y,
   float w,
   float h,
   float zNear,
   float zFar)
{
   D3D12_VIEWPORT viewport;
   viewport.TopLeftX = x;
   viewport.TopLeftY = y;
   viewport.Width = w;
   viewport.Height = h;
   viewport.MinDepth = zNear;
   viewport.MaxDepth = zFar;
   gDX.commandList->RSSetViewports(1, &viewport);
}

void
GX2SetScissor(uint32_t x,
   uint32_t y,
   uint32_t w,
   uint32_t h)
{
   D3D12_RECT rect;
   rect.left = x;
   rect.top = y;
   if (w >= 0xFFFFF) {
      rect.right = 0xFFFFF;
   } else {
      rect.right = x + w;
   }
   if (h >= 0xFFFFF) {
      rect.bottom = 0xFFFFF;
   } else {
      rect.bottom = y + h;
   }
   gDX.commandList->RSSetScissorRects(1, &rect);
}

#endif
