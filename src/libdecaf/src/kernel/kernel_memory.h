#pragma once
#include "kernel_enum.h"
#include <cstdint>
#include <libcpu/address.h>
#include <libcpu/pointer.h>

namespace kernel
{

void
initialiseVirtualMemory();

cpu::VirtualAddressRange
initialiseOverlayArena();

void
freeOverlayArena();

cpu::VirtualAddressRange
initialiseTilingApertures();

void
freeTilingApertures();

bool
initialiseAppMemory(uint32_t codeSize);

void
freeAppMemory();

cpu::VirtualAddressRange
getVirtualRange(VirtualRegion region);

cpu::PhysicalAddressRange
getPhysicalRange(PhysicalRegion region);

cpu::PhysicalAddressRange
getAvailPhysicalRange();

cpu::PhysicalAddressRange
getDataPhysicalRange();

} // namespace kernel
