#pragma once
#include <cstdint>
#include <libcpu/be2_struct.h>

namespace cafe::gx2::internal
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
captureCommandBuffer(virt_ptr<uint32_t> buffer,
                     uint32_t numWords);

void
captureCpuFlush(phys_addr buffer,
                uint32_t size);

void
captureGpuFlush(phys_addr buffer,
                uint32_t size);

void
captureSyncGpuRegisters(const uint32_t *registers,
                        uint32_t size);

} // namespace cafe::gx2::internal
