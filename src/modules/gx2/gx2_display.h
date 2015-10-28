#pragma once
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_enum.h"
#include "modules/gx2/gx2_surface.h"
#include "types.h"
#include "utils/be_val.h"

struct GX2Texture;
struct GX2ColorBuffer;

void
GX2SetTVEnable(BOOL enable);

void
GX2SetDRCEnable(BOOL enable);

void
GX2CalcTVSize(GX2TVRenderMode::Value tvRenderMode,
              GX2SurfaceFormat::Value surfaceFormat,
              GX2BufferingMode::Value bufferingMode,
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut);

void
GX2CalcDRCSize(GX2DrcRenderMode::Value drcRenderMode,
               GX2SurfaceFormat::Value surfaceFormat,
               GX2BufferingMode::Value bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut);

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode::Value tvRenderMode,
               GX2SurfaceFormat::Value surfaceFormat,
               GX2BufferingMode::Value bufferingMode);

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode::Value drcRenderMode,
                GX2SurfaceFormat::Value surfaceFormat,
                GX2BufferingMode::Value bufferingMode);

void
GX2SetTVScale(uint32_t x, uint32_t y);

void
GX2SetDRCScale(uint32_t x, uint32_t y);

uint32_t
GX2GetSwapInterval();

void
GX2SetSwapInterval(uint32_t interval);

GX2TVScanMode::Value
GX2GetSystemTVScanMode();

GX2DrcRenderMode::Value
GX2GetSystemDRCMode();

BOOL
GX2DrawDone();

void
GX2SwapScanBuffers();

void
GX2WaitForVsync();

void
GX2WaitForFlip();

void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync);

BOOL
GX2GetLastFrame(GX2ScanTarget::Value scanTarget, GX2Texture *texture);

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Value scanTarget, be_val<float> *gamma);

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget::Value scanTarget);

void
GX2VsyncCallback();
