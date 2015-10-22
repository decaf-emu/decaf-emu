#include "modules/gx2/gx2.h"
#ifdef GX2_DX12

#include "modules/gx2/gx2_displaylist.h"
#include "utils/virtual_ptr.h"

static virtual_ptr<GX2DisplayList>
gCurrentDisplayList = nullptr;

static uint32_t
gCurrentDisplayListSize = 0;


void
GX2BeginDisplayListEx(GX2DisplayList *displayList,
                      uint32_t size,
                      BOOL unk1)
{
   gCurrentDisplayList = displayList;
   gCurrentDisplayListSize = size;
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
   assert(gCurrentDisplayList == displayList);
   return gCurrentDisplayListSize;
}


void
GX2DirectCallDisplayList(GX2DisplayList *displayList,
                         uint32_t size)
{
   // TODO: GX2DirectCallDisplayList
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
   return !!gCurrentDisplayList;
}


BOOL
GX2GetCurrentDisplayList(be_val<uint32_t> *outDisplayList,
                         be_val<uint32_t> *outSize)
{
   if (!gCurrentDisplayList) {
      return FALSE;
   }

   *outDisplayList = gCurrentDisplayList.getAddress();
   *outSize = gCurrentDisplayListSize;
   return TRUE;
}


void
GX2CopyDisplayList(GX2DisplayList *displayList,
                   uint32_t size)
{
   auto dst = reinterpret_cast<uint8_t*>(gCurrentDisplayList.get()) + gCurrentDisplayListSize;
   memcpy(dst, displayList, size);
   gCurrentDisplayListSize += size;
}

#endif
