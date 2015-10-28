#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/gx2/gx2_renderstate.h"

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
}

void
GX2SetStencilMask(uint8_t unk1,
                  uint8_t unk2,
                  uint8_t unk3,
                  uint8_t unk4,
                  uint8_t unk5,
                  uint8_t unk6)
{
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
}

void
GX2SetColorControl(GX2LogicOp::Value logicOp,
                   uint8_t blendEnabled,
                   uint32_t unk2,
                   uint32_t unk3)
{
}

void
GX2SetBlendControl(GX2RenderTarget::Value target,
                   GX2BlendMode::Value colorSrcBlend,
                   GX2BlendMode::Value colorDstBlend,
                   GX2BlendCombineMode::Value colorCombine,
                   uint32_t unk1,
                   GX2BlendMode::Value alphaSrcBlend,
                   GX2BlendMode::Value alphaDstBlend,
                   GX2BlendCombineMode::Value alphaCombine)
{
}

void
GX2SetBlendConstantColor(float red,
                         float green,
                         float blue,
                         float alpha)
{
}

void
GX2SetAlphaTest(BOOL enabled,
                GX2CompareFunction::Value compare,
                float reference)
{
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
}

void
GX2SetAlphaToMask(BOOL enabled,
                  GX2AlphaToMaskMode::Value mode)
{
}

void
GX2SetViewport(float x1, float y1,
               float x2, float y2,
               float zNear, float zFar)
{
   assert(x1 == 0 && y1 == 0);
}

void
GX2SetScissor(uint32_t x1, uint32_t y1,
              uint32_t x2, uint32_t y2)
{
   assert(x1 == 0 && y1 == 0);
}

#endif
