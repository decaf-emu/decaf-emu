#include "coreinit.h"
#include "coreinit_overlayarena.h"
#include "cafe/kernel/cafe_kernel_mmu.h"

namespace cafe::coreinit
{

struct StaticOverlayArenaData
{
   be2_val<BOOL> enabled;
   be2_val<virt_addr> baseAddress;
   be2_val<uint32_t> size;
};

static virt_ptr<StaticOverlayArenaData>
sOverlayArenaData = nullptr;

BOOL
OSIsEnabledOverlayArena()
{
   return sOverlayArenaData->enabled;
}

void
OSEnableOverlayArena(uint32_t unk,
                     virt_ptr<virt_addr> outAddr,
                     virt_ptr<uint32_t> outSize)
{
   if (!sOverlayArenaData->enabled) {
      auto overlayArena = kernel::enableOverlayArena();
      sOverlayArenaData->baseAddress = overlayArena.first;
      sOverlayArenaData->size = overlayArena.second;
      sOverlayArenaData->enabled = TRUE;
   }

   OSGetOverlayArenaRange(outAddr, outSize);
}

void
OSDisableOverlayArena()
{
   if (sOverlayArenaData->enabled) {
      kernel::disableOverlayArena();
      sOverlayArenaData->baseAddress = cpu::VirtualAddress { 0u };
      sOverlayArenaData->size = 0u;
      sOverlayArenaData->enabled = FALSE;
   }
}

void
OSGetOverlayArenaRange(virt_ptr<virt_addr> outAddr,
                       virt_ptr<uint32_t> outSize)
{
   if (outAddr) {
      *outAddr = sOverlayArenaData->baseAddress;
   }

   if (outSize) {
      *outSize = sOverlayArenaData->size;
   }
}

void
Library::registerOverlayArenaSymbols()
{
   RegisterFunctionExport(OSIsEnabledOverlayArena);
   RegisterFunctionExport(OSEnableOverlayArena);
   RegisterFunctionExport(OSDisableOverlayArena);
   RegisterFunctionExport(OSGetOverlayArenaRange);

   RegisterDataInternal(sOverlayArenaData);
}

} // namespace cafe::coreinit
