#include "gx2.h"
#include "gx2_display.h"

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
GX2CalcTVSize(GX2TVRenderMode::Mode tvRenderMode,
              GX2SurfaceFormat::Format surfaceFormat,
              GX2BufferingMode::Mode bufferingMode,
              be_val<uint32_t> *size,
              be_val<uint32_t> *unkOut)
{
   // TODO: GX2CalcTVSize
   *size = 1920 * 1080;
}

void
GX2CalcDRCSize(GX2DrcRenderMode::Mode drcRenderMode,
               GX2SurfaceFormat::Format surfaceFormat,
               GX2BufferingMode::Mode bufferingMode,
               be_val<uint32_t> *size,
               be_val<uint32_t> *unkOut)
{
   // TODO: GX2CalcDRCSize
   *size = 854 * 480;
}

void
GX2SetTVBuffer(void *buffer,
               uint32_t size,
               GX2TVRenderMode::Mode tvRenderMode,
               GX2SurfaceFormat::Format surfaceFormat,
               GX2BufferingMode::Mode bufferingMode)
{
   // TODO: GX2SetTVBuffer
}

void
GX2SetDRCBuffer(void *buffer,
                uint32_t size,
                GX2DrcRenderMode::Mode drcRenderMode,
                GX2SurfaceFormat::Format surfaceFormat,
                GX2BufferingMode::Mode bufferingMode)
{
   // TODO: GX2SetDRCBuffer
}

void
GX2SetTVScale(uint32_t x,
              uint32_t y)
{
   // TODO: GX2SetTVScale
}

void
GX2SetDRCScale(uint32_t x,
               uint32_t y)
{
   // TODO: GX2SetDRCScale
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

GX2TVScanMode::Mode
GX2GetSystemTVScanMode()
{
   // TODO: GX2GetSystemTVScanMode
   return GX2TVScanMode::Last;
}

BOOL
GX2DrawDone()
{
   // TODO: GX2DrawDone
   return TRUE;
}

void
GX2WaitForVsync()
{
   // TODO: GX2WaitForVsync
}

void
GX2WaitForFlip()
{
   // TODO: GX2WaitForFlip
}

void
GX2GetSwapStatus(be_val<uint32_t> *unk1,
                 be_val<uint32_t> *unk2,
                 be_val<uint64_t> *unk3, // 64 bit is probably OSTime
                 be_val<uint64_t> *unk4) // 64 bit is probably OSTime
{
   // TODO: GX2GetSwapStatus
   // This gets us out of some loops!
   *unk1 = 1;
   *unk2 = 1;
}

BOOL
GX2GetLastFrame(GX2ScanTarget::Target scanTarget,
                GX2Texture *texture)
{
   // TODO: GX2GetLastFrame
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Target scanTarget,
                     be_val<float> *gamma)
{
   // TODO: GX2GetLastFrameGamma
   return FALSE;
}

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer,
                               GX2ScanTarget::Target scanTarget)
{
   // TODO: GX2CopyColorBufferToScanBuffer
}

void
GX2::registerDisplayFunctions()
{
   RegisterKernelFunction(GX2SetTVEnable);
   RegisterKernelFunction(GX2SetDRCEnable);
   RegisterKernelFunction(GX2CalcTVSize);
   RegisterKernelFunction(GX2SetTVBuffer);
   RegisterKernelFunction(GX2SetTVScale);
   RegisterKernelFunction(GX2CalcDRCSize);
   RegisterKernelFunction(GX2SetDRCBuffer);
   RegisterKernelFunction(GX2SetDRCScale);
   RegisterKernelFunction(GX2SetSwapInterval);
   RegisterKernelFunction(GX2GetSwapInterval);
   RegisterKernelFunction(GX2GetSystemTVScanMode);
   RegisterKernelFunction(GX2DrawDone);
   RegisterKernelFunction(GX2WaitForVsync);
   RegisterKernelFunction(GX2WaitForFlip);
   RegisterKernelFunction(GX2GetSwapStatus);
   RegisterKernelFunction(GX2GetLastFrame);
   RegisterKernelFunction(GX2GetLastFrameGamma);
   RegisterKernelFunction(GX2CopyColorBufferToScanBuffer);
}
