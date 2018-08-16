#pragma once
#include "gx2_enum.h"
#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

/**
 * \defgroup gx2 Clear Functions
 * \ingroup gx2
 * @{
 */

struct GX2ColorBuffer;
struct GX2DepthBuffer;

void
GX2ClearColor(virt_ptr<GX2ColorBuffer> colorBuffer,
              float red,
              float green,
              float blue,
              float alpha);

void
GX2ClearDepthStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                     GX2ClearFlags clearMode);

void
GX2ClearDepthStencilEx(virt_ptr<GX2DepthBuffer> depthBuffer,
                       float depth,
                       uint8_t stencil,
                       GX2ClearFlags clearMode);

void
GX2ClearBuffers(virt_ptr<GX2ColorBuffer> colorBuffer,
                virt_ptr<GX2DepthBuffer> depthBuffer,
                float red,
                float green,
                float blue,
                float alpha,
                GX2ClearFlags clearMode);

void
GX2ClearBuffersEx(virt_ptr<GX2ColorBuffer> colorBuffer,
                  virt_ptr<GX2DepthBuffer> depthBuffer,
                  float red,
                  float green,
                  float blue,
                  float alpha,
                  float depth,
                  uint8_t stencil,
                  GX2ClearFlags clearMode);

void
GX2SetClearDepth(virt_ptr<GX2DepthBuffer> depthBuffer,
                 float depth);

void
GX2SetClearStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                   uint8_t stencil);

void
GX2SetClearDepthStencil(virt_ptr<GX2DepthBuffer> depthBuffer,
                        float depth,
                        uint8_t stencil);

/** @} */

} // namespace cafe::gx2
