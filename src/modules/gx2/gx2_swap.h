#pragma once
#include "types.h"
#include "gx2_enum.h"
#include "modules/coreinit/coreinit_time.h"
#include "common/be_val.h"

namespace gx2
{

struct GX2ColorBuffer;
struct GX2Texture;

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget scanTarget);

void
GX2SwapScanBuffers();

BOOL
GX2GetLastFrame(GX2ScanTarget scanTarget, GX2Texture *texture);

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget, be_val<float> *gamma);

uint32_t
GX2GetSwapInterval();

void
GX2SetSwapInterval(uint32_t interval);

} // namespace gx2

