#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "dx12_state.h"
#include "modules/gx2/gx2_displaylist.h"
#include "utils/virtual_ptr.h"

void
GX2BeginDisplayListEx(GX2DisplayList *displayList,
                      uint32_t size,
                      BOOL unk1)
{
   uint32_t coreId = OSGetCoreId();
   auto &dlData = gDX.activeDisplayList[coreId];
   dlData.buffer = displayList;
   dlData.size = size;
   dlData.offset = 0;
}


void
GX2BeginDisplayList(GX2DisplayList *displayList,
   uint32_t size)
{
   GX2BeginDisplayListEx(displayList, size, TRUE);
}


uint32_t
GX2EndDisplayList(GX2DisplayList *displayList)
{
   uint32_t coreId = OSGetCoreId();
   auto &dlData = gDX.activeDisplayList[coreId];
   assert(dlData == displayList);

   uint32_t endOffset = dlData.offset;
   dlData.buffer = nullptr;
   dlData.size = 0;
   dlData.offset = 0;
   return endOffset;
}


void
GX2DirectCallDisplayList(GX2DisplayList *displayList,
                         uint32_t size)
{
   uint32_t endOffset = 0;
   CommandListRef cl = {
      reinterpret_cast<uint8_t*>(displayList),
      size,
      endOffset };
   commandListExecute(cl);
   if (endOffset != size) {
      // We need to make sure we processed the entire display list...
      throw;
   }
}


void
GX2CallDisplayList(GX2DisplayList *displayList,
                   uint32_t size)
{
   if (GX2GetDisplayListWriteStatus()) {
      GX2CopyDisplayList(displayList, size);
   } else {
      GX2DirectCallDisplayList(displayList, size);
   }
}


BOOL
GX2GetDisplayListWriteStatus()
{
   uint32_t coreId = OSGetCoreId();
   auto &dlData = gDX.activeDisplayList[coreId];
   return !!dlData.buffer;
}


BOOL
GX2GetCurrentDisplayList(be_val<uint32_t> *outDisplayList,
                         be_val<uint32_t> *outSize)
{
   uint32_t coreId = OSGetCoreId();
   auto &dlData = gDX.activeDisplayList[coreId];
   if (!dlData.buffer) {
      return FALSE;
   }

   *outDisplayList = memory_untranslate(dlData.buffer);
   *outSize = dlData.size;
   return TRUE;
}


void
GX2CopyDisplayList(GX2DisplayList *displayList,
                   uint32_t size)
{
   uint32_t coreId = OSGetCoreId();
   auto &dlData = gDX.activeDisplayList[coreId];

   // We do not currently handle DL_OVERFLOW events
   if (dlData.offset + size > dlData.size) {
      throw;
   }

   if (displayList == nullptr) {
      gLog->warn("GX2CopyDisplayList with nullptr");
      return;
   }

   auto dst = reinterpret_cast<uint8_t*>(dlData.buffer) + dlData.offset;
   memcpy(dst, displayList, size);
   dlData.offset += size;
}

#endif
