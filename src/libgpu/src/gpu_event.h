#pragma once
#include <cstdint>

namespace gpu
{

void
onFlip();

void
onSyncRegisters(const uint32_t *registers,
                uint32_t size);

} // namespace gpu
