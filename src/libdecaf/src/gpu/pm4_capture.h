#pragma once
#include <cstdint>
#include <string>

namespace pm4
{

struct Buffer;

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
captureCommandBuffer(pm4::Buffer *buffer);

void
captureCpuFlush(void *buffer,
                uint32_t size);

void
captureGpuFlush(void *buffer,
                uint32_t size);

void
captureSyncGpuRegisters(const uint32_t *registers,
                        uint32_t size);

} // namespace pm4
