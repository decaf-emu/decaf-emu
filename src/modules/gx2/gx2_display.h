#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "types.h"
#include "utils/be_val.h"

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
