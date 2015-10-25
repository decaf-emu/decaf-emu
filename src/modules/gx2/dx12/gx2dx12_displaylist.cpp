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
   gDX.activeDisplayList = displayList;
   gDX.activeDisplayListSize = size;
   gDX.activeDisplayListOffset = 0;
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
   assert(gDX.activeDisplayList == displayList);
   uint32_t endOffset = gDX.activeDisplayListOffset;
   gDX.activeDisplayList = nullptr;
   gDX.activeDisplayListSize = 0;
   gDX.activeDisplayListOffset = 0;
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
   return !!gDX.activeDisplayList;
}


BOOL
GX2GetCurrentDisplayList(be_val<uint32_t> *outDisplayList,
                         be_val<uint32_t> *outSize)
{
   if (!gDX.activeDisplayList) {
      return FALSE;
   }

   *outDisplayList = memory_untranslate(gDX.activeDisplayList);
   *outSize = gDX.activeDisplayListSize;
   return TRUE;
}


void
GX2CopyDisplayList(GX2DisplayList *displayList,
                   uint32_t size)
{
   // We do not currently handle DL_OVERFLOW events
   if (gDX.activeDisplayListOffset + size < gDX.activeDisplayListSize) {
      throw;
   }

   auto dst = reinterpret_cast<uint8_t*>(gDX.activeDisplayList) + gDX.activeDisplayListOffset;
   memcpy(dst, displayList, size);
   gDX.activeDisplayListOffset += size;
}

#endif
