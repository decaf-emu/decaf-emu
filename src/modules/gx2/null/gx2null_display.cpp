#include "modules/gx2/gx2.h"
#ifdef GX2_NULL

#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_display.h"

static uint32_t
gSwapInterval = 0;

static BOOL
gTVEnable = TRUE;

static BOOL
gDRCEnable = TRUE;

void
GX2SetTVEnable(BOOL enable)
{
   gTVEnable = enable;
}

void
GX2SetDRCEnable(BOOL enable)
{
   gDRCEnable = enable;
}

void
GX2CalcTVSize(GX2TVRenderMode::Value tvRenderMode,
              GX2SurfaceFormat::Value surfaceFormat,
              GX2BufferingMode::Value bufferingMode,
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut)
{
   *size = 1920 * 1080;
}

void
GX2CalcDRCSize(GX2DrcRenderMode::Value drcRenderMode,
               GX2SurfaceFormat::Value surfaceFormat,
               GX2BufferingMode::Value bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   *size = 854 * 480;
}

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode::Value tvRenderMode,
               GX2SurfaceFormat::Value surfaceFormat,
               GX2BufferingMode::Value bufferingMode)
{
}

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode::Value drcRenderMode,
                GX2SurfaceFormat::Value surfaceFormat,
                GX2BufferingMode::Value bufferingMode)
{
}

void
GX2SetTVScale(uint32_t x,
              uint32_t y)
{
}

void
GX2SetDRCScale(uint32_t x,
               uint32_t y)
{
}

uint32_t
GX2GetSwapInterval()
{
   return gSwapInterval;
}

void
GX2SetSwapInterval(uint32_t interval)
{
   gSwapInterval = interval;
}

GX2TVScanMode::Value
GX2GetSystemTVScanMode()
{
   return GX2TVScanMode::Last;
}

BOOL
GX2DrawDone()
{
   return TRUE;
}

void
GX2SwapScanBuffers()
{
}

void
GX2WaitForFlip()
{
}

void
GX2GetSwapStatus(be_val<uint32_t> *pSwapCount,
                 be_val<uint32_t> *pFlipCount,
                 be_val<OSTime> *pLastFlip,
                 be_val<OSTime> *pLastVsync)
{
   *pSwapCount = 0;
   *pFlipCount = 0;
}

BOOL
GX2GetLastFrame(GX2ScanTarget::Value scanTarget,
                GX2Texture *texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Value scanTarget,
                     be_val<float> *gamma)
{
   return FALSE;
}

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer,
                               GX2ScanTarget::Value scanTarget)
{
}

#endif
