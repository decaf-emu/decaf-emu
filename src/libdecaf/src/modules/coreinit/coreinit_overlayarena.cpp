#include "coreinit.h"
#include "coreinit_overlay.h"
#include "kernel/kernel_memory.h"

namespace coreinit
{

static bool
sOverlayArenaEnabled = false;

static cpu::VirtualAddress
sOverlayArenaBase = cpu::VirtualAddress { 0 };

static uint32_t
sOverlayArenaSize = 0;

BOOL
OSIsEnabledOverlayArena()
{
   return sOverlayArenaEnabled ? TRUE : FALSE;
}

void
OSEnableOverlayArena(uint32_t unk,
                     be_val<uint32_t> *addr,
                     be_val<uint32_t> *size)
{
   if (!sOverlayArenaEnabled) {
      auto bounds = kernel::initialiseOverlayArena();
      sOverlayArenaBase = bounds.start;
      sOverlayArenaSize = bounds.size;
      sOverlayArenaEnabled = true;
   }

   OSGetOverlayArenaRange(addr, size);
}

void
OSDisableOverlayArena()
{
   if (sOverlayArenaEnabled) {
      kernel::freeOverlayArena();
      sOverlayArenaBase = cpu::VirtualAddress { 0u };
      sOverlayArenaSize = 0;
      sOverlayArenaEnabled = false;
   }
}

void
OSGetOverlayArenaRange(be_val<uint32_t> *addr,
                       be_val<uint32_t> *size)
{
   if (addr) {
      *addr = sOverlayArenaBase.getAddress();
   }

   if (size) {
      *size = sOverlayArenaSize;
   }
}

void
Module::registerOverlayArenaFunctions()
{
   RegisterKernelFunction(OSIsEnabledOverlayArena);
   RegisterKernelFunction(OSEnableOverlayArena);
   RegisterKernelFunction(OSDisableOverlayArena);
   RegisterKernelFunction(OSGetOverlayArenaRange);
}

} // namespace coreinit
