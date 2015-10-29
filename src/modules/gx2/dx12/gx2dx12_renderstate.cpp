#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_renderstate.h"
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
   gLog->debug("unimplemented GX2SetDepthStencilControl");
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
   gLog->debug("unimplemented GX2SetStencilMask");
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
   gLog->debug("unimplemented GX2SetPolygonControl");
}

void
_GX2SetColorControl(GX2LogicOp::Value logicOp,
   uint8_t blendEnabled,
   uint32_t unk2,
   uint32_t unk3)
{
   auto &blendState = gDX.state.blendState;
   blendState.logicOp = logicOp;
   blendState.blendEnabled = blendEnabled;
}

void
GX2SetColorControl(GX2LogicOp::Value logicOp,
                   uint8_t blendEnabled,
                   uint32_t unk2,
                   uint32_t unk3)
{
   DX_DLCALL(_GX2SetColorControl, logicOp, blendEnabled, unk2, unk3);
}

void
_GX2SetBlendControl(GX2RenderTarget::Value target,
                   GX2BlendMode::Value colorSrcBlend,
                   GX2BlendMode::Value colorDstBlend,
                   GX2BlendCombineMode::Value colorCombine,
                   BOOL useAlphaBlend,
                   GX2BlendMode::Value alphaSrcBlend,
                   GX2BlendMode::Value alphaDstBlend,
                   GX2BlendCombineMode::Value alphaCombine)
{
   auto &blendState = gDX.state.targetBlendState[target];
   blendState.colorSrcBlend = colorSrcBlend;
   blendState.colorDstBlend = colorDstBlend;
   blendState.colorCombine = colorCombine;
   if (useAlphaBlend != 0) {
      blendState.alphaSrcBlend = alphaSrcBlend;
      blendState.alphaDstBlend = alphaDstBlend;
      blendState.alphaCombine = alphaCombine;
   } else {
      blendState.alphaSrcBlend = blendState.colorSrcBlend;
      blendState.alphaDstBlend = blendState.colorDstBlend;
      blendState.alphaCombine = blendState.colorCombine;
   }
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
   DX_DLCALL(_GX2SetBlendControl,
      target, colorSrcBlend, colorDstBlend, colorCombine,
      useAlphaBlend, alphaSrcBlend, alphaDstBlend, alphaCombine);
}

void
_GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha)
{
   auto &blendState = gDX.state.blendState;
   blendState.constColor[0] = red;
   blendState.constColor[1] = green;
   blendState.constColor[2] = blue;
   blendState.constColor[3] = alpha;
}

void
GX2SetBlendConstantColor(float red,
   float green,
   float blue,
   float alpha)
{
   DX_DLCALL(_GX2SetBlendConstantColor, red, green, blue, alpha);
}

void
_GX2SetAlphaTest(BOOL enabled,
                GX2CompareFunction::Value compare,
                float reference)
{
   auto &blendState = gDX.state.blendState;
   blendState.alphaTestEnabled = (enabled != 0);
   blendState.alphaFunc = compare;
   blendState.alphaRef = reference;
}

void
GX2SetAlphaTest(BOOL enabled,
   GX2CompareFunction::Value compare,
   float reference)
{
   DX_DLCALL(_GX2SetAlphaTest, enabled, compare, reference);
}

void
GX2SetTargetChannelMasks(GX2ChannelMask::Value target0,
                         GX2ChannelMask::Value target1,
                         GX2ChannelMask::Value target2,
                         GX2ChannelMask::Value target3,
                         GX2ChannelMask::Value target4,
                         GX2ChannelMask::Value target5,
                         GX2ChannelMask::Value target6)
{
   // TODO: GX2SetTargetChannelMasks
   gLog->debug("unimplemented GX2SetTargetChannelMasks");
}

void
GX2SetAlphaToMask(BOOL enabled,
                  GX2AlphaToMaskMode::Value mode)
{
   // TODO: GX2SetAlphaToMask
   gLog->debug("unimplemented GX2SetAlphaToMask");
}

void
_GX2SetViewport(float x,
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
GX2SetViewport(float x,
   float y,
   float w,
   float h,
   float zNear,
   float zFar)
{
   DX_DLCALL(_GX2SetViewport, x, y, w, h, zNear, zFar);
}

void
_GX2SetScissor(uint32_t x,
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

void
GX2SetScissor(uint32_t x,
   uint32_t y,
   uint32_t w,
   uint32_t h)
{
   DX_DLCALL(_GX2SetScissor, x, y, w, h);
}

#endif
