#pragma once
#include <cstdint>
#include <libcpu/address.h>
#include <libcpu/pointer.h>

namespace kernel
{

void
initialiseVirtualMemory();

cpu::VirtualAddressRange
initialiseLoaderMemory();

void
freeLoaderMemory();

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
getCodeBounds();

cpu::VirtualAddressRange
getForegroundBucketRange();

cpu::VirtualAddressRange
getLockedCacheBounds();

cpu::VirtualAddressRange
getMEM1Bound();

cpu::VirtualAddressRange
getMEM2Bound();

cpu::VirtualAddressRange
getSharedDataBounds();

cpu::VirtualAddressRange
getSystemHeapBounds();

cpu::VirtualAddressRange
getVirtualMapRange();

cpu::PhysicalAddressRange
getAvailPhysicalRange();

cpu::PhysicalAddressRange
getDataPhysicalRange();

} // namespace kernel
