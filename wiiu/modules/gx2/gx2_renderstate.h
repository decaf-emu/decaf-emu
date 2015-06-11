#pragma once
#include "systemtypes.h"

enum class LogicOp : uint32_t
{
   First = 0,     // GX2_LOGIC_OP_FIRST
   Last = 0xff   // GX2_LOGIC_OP_LAST
};

enum class CompareFunction : uint32_t
{
   First = 0,  // GX2_COMPARE_FIRST
   Last = 7    // GX2_COMPARE_LAST
};

enum class AlphaToMaskMode : uint32_t
{
   First = 0,  // GX2_ALPHA_TO_MASK_FIRST
   Last = 4    // GX2_ALPHA_TO_MASK_LAST
};

void
GX2SetDepthStencilControl(UNKNOWN_ARGS);

void
GX2SetStencilMask(UNKNOWN_ARGS);

void
GX2SetPolygonControl(UNKNOWN_ARGS);

void
GX2SetColorControl(LogicOp logicOp, uint32_t unk1, uint32_t unk2, uint32_t unk3);

// r3...r10?
void
GX2SetBlendControl(UNKNOWN_ARGS);

void
GX2SetBlendConstantColor(float red, float green, float blue, float alpha);

void
GX2SetAlphaTest(BOOL enabled, CompareFunction compare, float reference);

// r3...r10?
void
GX2SetTargetChannelMasks(UNKNOWN_ARGS);

void
GX2SetAlphaToMask(BOOL enabled, AlphaToMaskMode mode);
