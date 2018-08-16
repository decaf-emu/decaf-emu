#pragma once
#include "gx2_enum.h"

#include <libcpu/be2_struct.h>

namespace cafe::gx2
{

struct GX2ColorBuffer;
struct GX2Texture;

void
GX2CopyColorBufferToScanBuffer(virt_ptr<GX2ColorBuffer> buffer,
                               GX2ScanTarget scanTarget);

void
GX2SwapScanBuffers();

BOOL
GX2GetLastFrame(GX2ScanTarget scanTarget,
                virt_ptr<GX2Texture> texture);

BOOL
GX2GetLastFrameGamma(GX2ScanTarget scanTarget,
                     virt_ptr<float> outGamma);

uint32_t
GX2GetSwapInterval();

void
GX2SetSwapInterval(uint32_t interval);

} // namespace cafe::gx2

