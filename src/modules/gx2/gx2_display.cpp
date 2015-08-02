#include "gx2.h"
#include "gx2_display.h"

static uint32_t gSwapInterval;
static BOOL gTVEnable;
static BOOL gDRCEnable;

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
GX2CalcTVSize(GX2TVRenderMode::Mode tvRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut)
{
   *size = 1920 * 1080;
}
void
GX2CalcDRCSize(GX2DrcRenderMode::Mode drcRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut)
{
   *size = 854 * 480;
}

void
GX2SetTVBuffer(p32<void> buffer, uint32_t size, GX2TVRenderMode::Mode tvRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode)
{
}

void
GX2SetDRCBuffer(p32<void> buffer, uint32_t size, GX2DrcRenderMode::Mode drcRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode)
{
}

void
GX2SetTVScale(uint32_t x, uint32_t y)
{
}

void
GX2SetDRCScale(uint32_t x, uint32_t y)
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

GX2TVScanMode::Mode
GX2GetSystemTVScanMode()
{
   // My console returns 7
   return GX2TVScanMode::Last;
}

BOOL
GX2DrawDone()
{
   return TRUE;
}

void
GX2WaitForVsync()
{
}

void
GX2WaitForFlip()
{
}

void
GX2GetSwapStatus(be_val<uint32_t> *unk1, be_val<uint32_t> *unk2, be_val<uint64_t> *unk3, be_val<uint64_t> *unk4)
{
   // unk3 and unk4 are probably OSTime because they are 64 bit

   // This gets us out of loops!
   *unk1 = 1;
   *unk2 = 1;
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
}
