#include "cafe_loader_globals.h"
#include "cafe_loader_heap.h"
#include "cafe_loader_log.h"
#include "cafe_loader_loaded_rpl.h"
#include "cafe_loader_purge.h"

namespace cafe::loader::internal
{

void
LiPurgeOneUnlinkedModule(virt_ptr<LOADED_RPL> rpl)
{
   auto globals = getGlobalStorage();
   if (rpl->globals != globals) {
      Loader_ReportWarn("*** Purge of module in foreign process!");
      return;
   }

   if (!(rpl->loadStateFlags & LoaderStateFlags_Unk0x20000000)) {
      if (rpl->textBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->textBuffer,
                                  rpl->textBufferSize);
      }

      if (rpl->compressedRelocationsBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->compressedRelocationsBuffer,
                                  rpl->compressedRelocationsBufferSize);
      }

      if (rpl->moduleNameBuffer && rpl->moduleNameBufferSize) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->moduleNameBuffer,
                                  rpl->moduleNameBufferSize);
      }

      if (rpl->pathBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->pathBuffer,
                                  rpl->pathBufferSize);
      }

      if (rpl->sectionHeaderBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->sectionHeaderBuffer,
                                  rpl->sectionHeaderBufferSize);
      }

      if (rpl->fileInfoBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->fileInfoBuffer,
                                  rpl->fileInfoBufferSize);
      }

      if (rpl->crcBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->crcBuffer,
                                  rpl->crcBufferSize);
      }

      if (rpl->sectionAddressBuffer) {
         LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                                  rpl->sectionAddressBuffer,
                                  rpl->sectionAddressBufferSize);
      }
   }

   std::memset(rpl.get(), 0, sizeof(LOADED_RPL));
   LiCacheLineCorrectFreeEx(globals->processCodeHeap,
                            rpl,
                            rpl->selfBufferSize);
}

} // namespace cafe::loader::internal
