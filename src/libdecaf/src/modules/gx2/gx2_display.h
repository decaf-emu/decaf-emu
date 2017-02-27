#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "ppcutils/wfunc_ptr.h"

#include <common/be_val.h>
#include <common/cbool.h>
#include <cstdint>

namespace gx2
{

using GX2DRCConnectCallbackFunction = wfunc_ptr<void, uint32_t, BOOL>;

void
GX2SetTVEnable(BOOL enable);

void
GX2SetDRCEnable(BOOL enable);

void
GX2CalcTVSize(GX2TVRenderMode tvRenderMode,
              GX2SurfaceFormat surfaceFormat,
              GX2BufferingMode bufferingMode,
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut);

void
GX2CalcDRCSize(GX2DrcRenderMode drcRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut);

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode tvRenderMode,
               GX2SurfaceFormat surfaceFormat,
               GX2BufferingMode bufferingMode);

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode drcRenderMode,
                GX2SurfaceFormat surfaceFormat,
                GX2BufferingMode bufferingMode);

void
GX2SetTVScale(uint32_t x, uint32_t y);

void
GX2SetDRCScale(uint32_t x, uint32_t y);

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

struct ScreenBufferInfo
{
   void *buffer = nullptr;
   uint32_t size;
   GX2DrcRenderMode drcRenderMode;
   GX2TVRenderMode tvRenderMode;
   GX2SurfaceFormat surfaceFormat;
   GX2BufferingMode bufferingMode;
   unsigned width;
   unsigned height;
};

//! Returns data set by GX2SetTVBuffer
ScreenBufferInfo *
getTvBufferInfo();

//! Returns data set by GX2SetDRCBuffer
ScreenBufferInfo *
getDrcBufferInfo();

} // namespace internal

} // namespace gx2
