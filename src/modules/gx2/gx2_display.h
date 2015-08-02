#pragma once
#include "systemtypes.h"

struct GX2Texture;
struct GX2ColorBuffer;

namespace GX2DrcRenderMode
{
enum Mode : uint32_t
{
   First = 0,
   Last = 3
};
}

namespace GX2TVRenderMode
{
enum Mode : uint32_t
{
   First = 0,
   Last = 5
};
}

namespace GX2SurfaceFormat
{
enum Format : uint32_t
{
   First = 1,
   Last = 0x83f
};
}

namespace GX2BufferingMode
{
enum Mode : uint32_t
{
   First = 1,
   Last = 4
};
}

namespace GX2TVScanMode
{
enum Mode : uint32_t
{
   None = 0,
   First = 0,
   Last = 7
};
}

namespace GX2ScanTarget
{
enum Target
{
   First = 1,
   Last = 8
};
}

void
GX2SetTVEnable(BOOL enable);

void
GX2SetDRCEnable(BOOL enable);

void
GX2CalcTVSize(GX2TVRenderMode::Mode tvRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut);

void
GX2CalcDRCSize(GX2DrcRenderMode::Mode drcRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut);

void
GX2SetTVBuffer(void *buffer, uint32_t size, GX2TVRenderMode::Mode tvRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode);

void
GX2SetDRCBuffer(void *buffer, uint32_t size, GX2DrcRenderMode::Mode drcRenderMode, GX2SurfaceFormat::Format surfaceFormat, GX2BufferingMode::Mode bufferingMode);

void
GX2SetTVScale(uint32_t x, uint32_t y);

void
GX2SetDRCScale(uint32_t x, uint32_t y);

uint32_t
GX2GetSwapInterval();

void
GX2SetSwapInterval(uint32_t interval);

GX2TVScanMode::Mode
GX2GetSystemTVScanMode();

BOOL
GX2DrawDone();

void
GX2WaitForVsync();

void
GX2WaitForFlip();

void
GX2GetSwapStatus(be_val<uint32_t> *unk1, be_val<uint32_t> *unk2, be_val<uint64_t> *unk3, be_val<uint64_t> *unk4);

BOOL
GX2GetLastFrame(GX2ScanTarget::Target scanTarget, GX2Texture *texture);

BOOL
GX2GetLastFrameGamma(GX2ScanTarget::Target scanTarget, be_val<float> *gamma);

void
GX2CopyColorBufferToScanBuffer(GX2ColorBuffer *buffer, GX2ScanTarget::Target scanTarget);
