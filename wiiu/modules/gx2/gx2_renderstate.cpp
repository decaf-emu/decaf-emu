#include "gx2.h"
#include "gx2_renderstate.h"

void
GX2SetDepthStencilControl(UNKNOWN_ARGS)
{
}

void
GX2SetStencilMask(UNKNOWN_ARGS)
{
}

void
GX2SetPolygonControl(UNKNOWN_ARGS)
{
}

void
GX2SetColorControl(LogicOp logicOp, uint32_t unk1, uint32_t unk2, uint32_t unk3)
{
}

// r3...r10?
void
GX2SetBlendControl(UNKNOWN_ARGS)
{
}

void
GX2SetBlendConstantColor(float red, float green, float blue, float alpha)
{
}

void
GX2SetAlphaTest(BOOL enabled, CompareFunction compare, float reference)
{
}

void
GX2SetTargetChannelMasks(UNKNOWN_ARGS)
{
}

void
GX2SetAlphaToMask(BOOL enabled, AlphaToMaskMode mode)
{
}

void
GX2::registerRenderStateFunctions()
{
   RegisterSystemFunction(GX2SetDepthStencilControl);
   RegisterSystemFunction(GX2SetStencilMask);
   RegisterSystemFunction(GX2SetPolygonControl);
   RegisterSystemFunction(GX2SetColorControl);
   RegisterSystemFunction(GX2SetBlendControl);
   RegisterSystemFunction(GX2SetBlendConstantColor);
   RegisterSystemFunction(GX2SetAlphaTest);
   RegisterSystemFunction(GX2SetTargetChannelMasks);
   RegisterSystemFunction(GX2SetAlphaToMask);
}
