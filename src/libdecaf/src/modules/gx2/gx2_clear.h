#pragma once
#include "modules/gx2/gx2_enum.h"
#include <cstdint>

namespace gx2
{

/**
 * \defgroup gx2_clear Clear Functions
 * \ingroup gx2
 * @{
 */

struct GX2ColorBuffer;
struct GX2DepthBuffer;

void
GX2ClearColor(GX2ColorBuffer *colorBuffer,
              float red,
              float green,
              float blue,
              float alpha);

void
GX2ClearDepthStencil(GX2DepthBuffer *depthBuffer,
                     GX2ClearFlags clearMode);

void
GX2ClearDepthStencilEx(GX2DepthBuffer *depthBuffer,
                       float depth, uint8_t stencil,
                       GX2ClearFlags clearMode);

void
GX2ClearBuffers(GX2ColorBuffer *colorBuffer,
                GX2DepthBuffer *depthBuffer,
                float red, float green, float blue, float alpha,
                GX2ClearFlags clearMode);

void
GX2ClearBuffersEx(GX2ColorBuffer *colorBuffer,
                  GX2DepthBuffer *depthBuffer,
                  float red, float green, float blue, float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags clearMode);

void
GX2SetClearDepth(GX2DepthBuffer *depthBuffer,
                 float depth);

void
GX2SetClearStencil(GX2DepthBuffer *depthBuffer,
                   uint8_t stencil);

void
GX2SetClearDepthStencil(GX2DepthBuffer *depthBuffer,
                        float depth,
                        uint8_t stencil);

/** @} */

} // namespace gx2
