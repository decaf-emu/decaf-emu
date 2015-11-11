#include "gx2_swap.h"

static uint32_t
gSwapInterval = 1;

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget::Value scanTarget)
{
}

void
GX2SwapBuffers(GX2ColorBuffer *renderBuffer)
{
}

void
GX2SwapScanBuffers()
{
}

BOOL
GX2GetLastFrame(GX2ScanTarget::Value scanTarget, GX2Texture *texture)
{
   return FALSE;
}

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Value scanTarget, be_val<float> *gamma)
{
   return FALSE;
}

void
GX2GetSwapStatus(be_val<uint32_t> *swapCount,
                 be_val<uint32_t> *flipCount,
                 be_val<OSTime> *lastFlip,
                 be_val<OSTime> *lastVsync)
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
