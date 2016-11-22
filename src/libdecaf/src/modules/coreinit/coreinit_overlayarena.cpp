#include "coreinit.h"
#include "coreinit_overlay.h"
#include <libcpu/mem.h>

namespace coreinit
{

static bool
sOverlayArenaEnabled = false;

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
      if (!mem::commit(mem::OverlayArenaBase, mem::OverlayArenaSize)) {
         decaf_abort("Failed to allocate loader overlay memory");
      }

      sOverlayArenaEnabled = true;
   }

   OSGetOverlayArenaRange(addr, size);
}

void
OSDisableOverlayArena()
{
   if (sOverlayArenaEnabled) {
      mem::uncommit(mem::OverlayArenaBase, mem::OverlayArenaSize);
      sOverlayArenaEnabled = false;
   }
}

void
OSGetOverlayArenaRange(be_val<uint32_t> *addr,
                       be_val<uint32_t> *size)
{
   if (addr) {
      *addr = mem::OverlayArenaBase;
   }

   if (size) {
      *size = mem::OverlayArenaSize;
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
