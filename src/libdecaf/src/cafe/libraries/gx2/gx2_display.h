#pragma once
#include "gx2_enum.h"
#include "gx2_surface.h"
#include "gx2_texture.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

using GX2DRCConnectCallbackFunction = virt_func_ptr<void(uint32_t, BOOL)>;

void
GX2CalcTVSize(GX2TVRenderMode tvRenderMode,
              GX2SurfaceFormat surfaceFormat,
              GX2BufferingMode bufferingMode,
              virt_ptr<uint32_t> outSize,
              virt_ptr<uint32_t> outUnk);

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               virt_ptr<uint32_t> outSize,
               virt_ptr<uint32_t> outUnk);

void
GX2CopyColorBufferToScanBuffer(virt_ptr<GX2ColorBuffer> buffer,
                               GX2ScanTarget scanTarget);

BOOL
GX2GetLastFrame(GX2ScanTarget scanTarget,
                virt_ptr<GX2Texture> texture);

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget,
                     virt_ptr<float> outGamma);

GX2TVScanMode
GX2GetSystemTVScanMode();

GX2DrcRenderMode
GX2GetSystemDRCMode();

GX2AspectRatio
GX2GetSystemTVAspectRatio();

uint32_t
GX2GetSwapInterval();

BOOL
GX2IsVideoOutReady();

void
GX2SetDRCBuffer(virt_ptr<void> buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode);

GX2DRCConnectCallbackFunction
GX2SetDRCConnectCallback(uint32_t id,
                         GX2DRCConnectCallbackFunction callback);

void
GX2SetDRCEnable(BOOL enable);

void
GX2SetDRCScale(uint32_t x,
               uint32_t y);

void
GX2SetSwapInterval(uint32_t interval);

void
GX2SetTVBuffer(virt_ptr<void> buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode);

void
GX2SetTVEnable(BOOL enable);

void
GX2SetTVScale(uint32_t x,
              uint32_t y);

void
GX2SwapScanBuffers();

namespace internal
{

void
initialiseDisplay();

virt_ptr<GX2Surface>
getTvScanBuffer();

virt_ptr<GX2Surface>
getDrcScanBuffer();

} // namespace internal

} // namespace cafe::gx2
