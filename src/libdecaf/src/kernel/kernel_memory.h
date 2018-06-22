#pragma once
#include "kernel_enum.h"
#include <cstdint>
#include <libcpu/address.h>
#include <libcpu/pointer.h>

namespace kernel
{

enum class PhysicalRegion
{
   Invalid = -1,
   MEM1 = 0,
   LockedCache,
   MEM0,
   MEM0CafeKernel,
   MEM0IosKernel,
   MEM0IosMcp,
   MEM0IosCrypto,
   MEM2,
   MEM2IosUsb,
   MEM2IosFs,
   MEM2IosPad,
   MEM2IosNet,
   MEM2IosAcp,
   MEM2IosNsec,
   MEM2IosNim,
   MEM2IosFpd,
   MEM2IosTest,
   MEM2IosAuxil,
   MEM2IosBsp,
   MEM2ForegroundBucket,
   MEM2SharedData,
   MEM2CafeKernelWorkAreaHeap,
   MEM2LoaderHeap,
   MEM2IosSharedHeap,
   MEM2IosNetIobuf,
   MEM2IosFsRamdisk,
   MEM2HomeMenu,
   MEM2Root,
   MEM2CafeOS,
   MEM2ErrorDisplay,
   MEM2OverlayApp,
   MEM2MainApp,
   MEM2DevKit,
   TilingApertures,
   SRAM1,
   SRAM1C2W,
   SRAM0,
   SRAM0IosKernel,
};

enum class VirtualRegion
{
   Invalid = -1,
   CafeOS = 0,
   MainAppCode,
   MainAppData,
   OverlayAppCode,
   OverlayAppData,
   OverlayArena,
   TilingApertures,
   VirtualMapRange,
   ForegroundBucket,
   MEM1,
   SharedData,
   LockedCache,
   KernelStatic,
   KernelWorkAreaHeap,
};

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
initialiseAppMemory(uint32_t codeSize,
                    uint32_t codeGenSize,
                    uint32_t availSize);

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
