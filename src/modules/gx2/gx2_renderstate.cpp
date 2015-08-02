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
GX2SetColorControl(GX2LogicOp::Op logicOp, uint32_t unk1, uint32_t unk2, uint32_t unk3)
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
GX2SetAlphaTest(BOOL enabled, GX2CompareFunction::Func compare, float reference)
{
}

void
GX2SetTargetChannelMasks(UNKNOWN_ARGS)
{
}

void
GX2SetAlphaToMask(BOOL enabled, GX2AlphaToMaskMode::Mode mode)
{
}

void
GX2SetViewport(float x1, float y1, float x2, float y2, float zNear, float zFar)
{
   // TODO: Ensure this is not x,y,w,h
   assert(x1 == 0 && y1 == 0);
}

void
GX2SetScissor(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2)
{
   // TODO: Ensure this is not x,y,w,h
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
