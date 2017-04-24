#pragma once
#include "gx2_internal_cbpool.h"

#include <cstdint>

namespace gx2
{

namespace internal
{

enum class CaptureState
{
   Disabled,
   WaitStartNextFrame,
   Enabled,
   WaitEndNextFrame,
};

bool
captureStartAtNextSwap();

void
captureStopAtNextSwap();

bool
captureNextFrame();

CaptureState
captureState();

void
captureSwap();

void
captureCommandBuffer(CommandBuffer *buffer);

void
captureCpuFlush(void *buffer,
                uint32_t size);

void
captureGpuFlush(void *buffer,
                uint32_t size);

void
captureSyncGpuRegisters(const uint32_t *registers,
                        uint32_t size);

} // namespace internal

} // namespace gx2
