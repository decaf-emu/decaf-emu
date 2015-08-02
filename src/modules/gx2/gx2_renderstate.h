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
enum class Mode : uint32_t
{
   First = 0,  // GX2_ALPHA_TO_MASK_FIRST
   Last = 4    // GX2_ALPHA_TO_MASK_LAST
};
}

void
GX2SetDepthStencilControl(UNKNOWN_ARGS);

void
GX2SetStencilMask(UNKNOWN_ARGS);

void
GX2SetPolygonControl(UNKNOWN_ARGS);

void
GX2SetColorControl(GX2LogicOp::Op logicOp, uint32_t unk1, uint32_t unk2, uint32_t unk3);

// r3...r10?
void
GX2SetBlendControl(UNKNOWN_ARGS);

void
GX2SetBlendConstantColor(float red, float green, float blue, float alpha);

void
GX2SetAlphaTest(BOOL enabled, GX2CompareFunction::Func compare, float reference);

// r3...r10?
void
GX2SetTargetChannelMasks(UNKNOWN_ARGS);

void
GX2SetAlphaToMask(BOOL enabled, GX2AlphaToMaskMode::Mode mode);

void
GX2SetViewport(float x1, float y1, float x2, float y2, float zNear, float zFar);

void
GX2SetScissor(uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4);
