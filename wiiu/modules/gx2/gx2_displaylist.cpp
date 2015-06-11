#include "gx2.h"
#include "gx2_displaylist.h"

static GX2DisplayList *
gCurrentDisplayList;

static uint32_t
gCurrentDisplayListSize;

void
GX2BeginDisplayListEx(GX2DisplayList *displayList, uint32_t size, BOOL unk1)
{
   gCurrentDisplayList = displayList;
   gCurrentDisplayListSize = size;
}

void
GX2BeginDisplayList(GX2DisplayList *displayList, uint32_t size)
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
GX2DirectCallDisplayList(GX2DisplayList *displayList, uint32_t size)
{
   // TODO: Add to current GPU command buffer
}

void
GX2CallDisplayList(GX2DisplayList *displayList, uint32_t size)
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
GX2GetCurrentDisplayList(p32<be_val<uint32_t>> outDisplayList, p32<be_val<uint32_t>> outSize)
{
   if (!gCurrentDisplayList) {
      return FALSE;
   }

   *outDisplayList = make_p32<void>(gCurrentDisplayList).value;
   *outSize = gCurrentDisplayListSize;
   return TRUE;
}

void
GX2CopyDisplayList(GX2DisplayList *displayList, uint32_t size)
{
   auto dst = reinterpret_cast<uint8_t*>(gCurrentDisplayList) + gCurrentDisplayListSize;
   memcpy(dst, displayList, size);
   gCurrentDisplayListSize += size;
}

void
GX2::registerDisplayListFunctions()
{
   RegisterSystemFunction(GX2BeginDisplayListEx);
   RegisterSystemFunction(GX2BeginDisplayList);
   RegisterSystemFunction(GX2EndDisplayList);
   RegisterSystemFunction(GX2DirectCallDisplayList);
   RegisterSystemFunction(GX2CallDisplayList);
   RegisterSystemFunction(GX2GetDisplayListWriteStatus);
   RegisterSystemFunction(GX2GetCurrentDisplayList);
   RegisterSystemFunction(GX2CopyDisplayList);
}
