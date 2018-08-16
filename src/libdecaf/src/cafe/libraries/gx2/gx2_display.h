#pragma once
#include "gx2_enum.h"
#include "gx2_surface.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

using GX2DRCConnectCallbackFunction = virt_func_ptr<void(uint32_t, BOOL)>;

void
GX2SetTVEnable(BOOL enable);

void
GX2SetDRCEnable(BOOL enable);

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
GX2SetTVBuffer(virt_ptr<void> buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode);

void
GX2SetDRCBuffer(virt_ptr<void> buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode);

void
GX2SetTVScale(uint32_t x,
              uint32_t y);

void
GX2SetDRCScale(uint32_t x,
               uint32_t y);

GX2TVScanMode
GX2GetSystemTVScanMode();

GX2DrcRenderMode
GX2GetSystemDRCMode();

GX2AspectRatio
GX2GetSystemTVAspectRatio();

BOOL
GX2IsVideoOutReady();

GX2DRCConnectCallbackFunction
GX2SetDRCConnectCallback(uint32_t id,
                         GX2DRCConnectCallbackFunction callback);

namespace internal
{

void
initialiseDisplay();

struct ScreenBufferInfo
{
   virt_ptr<void> buffer = nullptr;
   uint32_t size;
   GX2DrcRenderMode drcRenderMode;
   GX2TVRenderMode tvRenderMode;
   GX2SurfaceFormat surfaceFormat;
   GX2BufferingMode bufferingMode;
   unsigned width;
   unsigned height;
};

//! Returns data set by GX2SetTVBuffer
virt_ptr<ScreenBufferInfo>
getTvBufferInfo();

//! Returns data set by GX2SetDRCBuffer
virt_ptr<ScreenBufferInfo>
getDrcBufferInfo();

} // namespace internal

} // namespace cafe::gx2
