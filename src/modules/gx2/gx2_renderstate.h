#pragma once
#include "systemtypes.h"

namespace GX2LogicOp
{
enum Op : uint32_t
{
   First = 0,     // GX2_LOGIC_OP_FIRST
   Last = 0xff   // GX2_LOGIC_OP_LAST
};
}

namespace GX2CompareFunction
{
enum Func : uint32_t
{
   First = 0,  // GX2_COMPARE_FIRST
   Last = 7    // GX2_COMPARE_LAST
};
}

namespace GX2AlphaToMaskMode
{
enum Mode : uint32_t
{
   First = 0,  // GX2_ALPHA_TO_MASK_FIRST
   Last = 4    // GX2_ALPHA_TO_MASK_LAST
};
}

namespace GX2RenderTarget
{
enum Target : uint32_t
{
   First = 0,
   Last = 7
};
}

namespace GX2BlendMode
{
enum Mode
{
   First = 0,
   Last = 20
};
}

namespace GX2BlendCombineMode
{
enum Mode
{
   First = 0,
   Last = 4
};
}

namespace GX2ChannelMask
{
enum Mask
{
   First = 0,
   Last = 0xf
};
}

void
GX2SetDepthStencilControl(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void
GX2SetStencilMask(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);

void
GX2SetPolygonControl(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void
GX2SetColorControl(GX2LogicOp::Op logicOp, uint32_t unk1, uint32_t unk2, uint32_t unk3);

void
GX2SetBlendControl(GX2RenderTarget::Target target,
                   GX2BlendMode::Mode colorSrcBlend,
                   GX2BlendMode::Mode colorDstBlend,
                   GX2BlendCombineMode::Mode colorCombine,
                   uint32_t unk1,
                   GX2BlendMode::Mode alphaSrcBlend,
                   GX2BlendMode::Mode alphaDstBlend,
                   GX2BlendCombineMode::Mode alphaCombine);

void
GX2SetBlendConstantColor(float red, float green, float blue, float alpha);

void
GX2SetAlphaTest(BOOL enabled, GX2CompareFunction::Func compare, float reference);

void
GX2SetTargetChannelMasks(GX2ChannelMask::Mask target0,
                         GX2ChannelMask::Mask target1,
                         GX2ChannelMask::Mask target2,
                         GX2ChannelMask::Mask target3,
                         GX2ChannelMask::Mask target4,
                         GX2ChannelMask::Mask target5,
                         GX2ChannelMask::Mask target6);

void
GX2SetAlphaToMask(BOOL enabled, GX2AlphaToMaskMode::Mode mode);

void
GX2SetViewport(float x1, float y1, float x2, float y2, float zNear, float zFar);

void
GX2SetScissor(uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4);
