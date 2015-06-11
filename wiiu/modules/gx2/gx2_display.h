#pragma once
#include "systemtypes.h"

enum class DrcRenderMode : uint32_t
{
   First = 0,
   Last = 3
};

enum class TvRenderMode : uint32_t
{
   First = 0,
   Last = 5
};

enum class SurfaceFormat : uint32_t
{
   First = 1,
   Last = 0x83f
};

enum class BufferingMode : uint32_t
{
   First = 1,
   Last = 4
};

void
GX2CalcTVSize(TvRenderMode tvRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut);

void
GX2CalcDRCSize(DrcRenderMode drcRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode, be_val<uint32_t> *size, be_val<uint32_t> *unkOut);

void
GX2SetTVBuffer(p32<void> buffer, uint32_t size, TvRenderMode tvRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode);

void
GX2SetDRCBuffer(p32<void> buffer, uint32_t size, DrcRenderMode drcRenderMode, SurfaceFormat surfaceFormat, BufferingMode bufferingMode);

void
GX2SetTVScale(uint32_t x, uint32_t y);

void
GX2SetDRCScale(uint32_t x, uint32_t y);

uint32_t
GX2GetSwapInterval();

void
GX2SetSwapInterval(uint32_t interval);
