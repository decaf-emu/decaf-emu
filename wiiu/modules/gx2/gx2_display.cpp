#include "gx2.h"
#include "gx2_display.h"

static uint32_t gSwapInterval;

void
GX2CalcTVSize(TvRenderMode tvRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut)
{
   *size = 1920 * 1080;
}
void
GX2CalcDRCSize(DrcRenderMode drcRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut)
{
   *size = 854 * 480;
}

void
GX2SetTVBuffer(p32<void> buffer, uint32_t size, TvRenderMode tvRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode)
{
}

void
GX2SetDRCBuffer(p32<void> buffer, uint32_t size, DrcRenderMode drcRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode)
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

void
GX2::registerDisplayFunctions()
{
   RegisterSystemFunction(GX2CalcTVSize);
   RegisterSystemFunction(GX2SetTVBuffer);
   RegisterSystemFunction(GX2SetTVScale);
   RegisterSystemFunction(GX2CalcDRCSize);
   RegisterSystemFunction(GX2SetDRCBuffer);
   RegisterSystemFunction(GX2SetDRCScale);
   RegisterSystemFunction(GX2SetSwapInterval);
   RegisterSystemFunction(GX2GetSwapInterval);
}
