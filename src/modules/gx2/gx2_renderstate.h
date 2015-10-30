#pragma once
#include "modules/gx2/gx2_enum.h"
#include "types.h"

void
GX2SetDepthStencilControl(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void
GX2SetStencilMask(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);

void
GX2SetPolygonControl(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

void
GX2SetColorControl(GX2LogicOp::Value logicOp, uint8_t blendEnabled, uint32_t unk2, uint32_t unk3);

void
GX2SetBlendControl(GX2RenderTarget::Value target,
                   GX2BlendMode::Value colorSrcBlend,
                   GX2BlendMode::Value colorDstBlend,
                   GX2BlendCombineMode::Value colorCombine,
                   BOOL unk1,
                   GX2BlendMode::Value alphaSrcBlend,
                   GX2BlendMode::Value alphaDstBlend,
                   GX2BlendCombineMode::Value alphaCombine);

void
GX2SetBlendConstantColor(float red, float green, float blue, float alpha);

void
GX2SetAlphaTest(BOOL enabled, GX2CompareFunction::Value compare, float reference);

void
GX2SetTargetChannelMasks(GX2ChannelMask::Value target0,
                         GX2ChannelMask::Value target1,
                         GX2ChannelMask::Value target2,
                         GX2ChannelMask::Value target3,
                         GX2ChannelMask::Value target4,
                         GX2ChannelMask::Value target5,
                         GX2ChannelMask::Value target6,
                         GX2ChannelMask::Value target7);

void
GX2SetAlphaToMask(BOOL enabled, GX2AlphaToMaskMode::Value mode);

void
GX2SetViewport(float x1, float y1, float x2, float y2, float zNear, float zFar);

void
GX2SetScissor(uint32_t r1, uint32_t r2, uint32_t r3, uint32_t r4);

void
GX2SetPixelSamplerBorderColor(uint32_t samplerId,
   float red, float green, float blue, float alpha);
