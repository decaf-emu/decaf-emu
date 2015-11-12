#include "gx2_cbpool.h"
#include "gx2_displaylist.h"
#include "gx2_state.h"
#include "gpu/driver/commandqueue.h"
#include "gpu/pm4_writer.h"
#include "modules/coreinit/coreinit_core.h"
#include <array>

static std::array<pm4::Buffer, CoreCount>
gActiveDisplayList;

void
GX2BeginDisplayList(void *displayList, uint32_t size)
{
   GX2BeginDisplayListEx(displayList, size, TRUE);
}

void
GX2BeginDisplayListEx(void *displayList, uint32_t size, BOOL unk1)
{
   auto core = OSGetCoreId();
   auto &active = gActiveDisplayList[core];

   // Set active display list
   active.buffer = reinterpret_cast<uint32_t*>(displayList);
   active.curSize = 0;
   active.maxSize = size;

   // Set active command buffer to the display list
   gx2::internal::setUserCommandBuffer(&active);
}

uint32_t
GX2EndDisplayList(void *displayList)
{
   auto core = OSGetCoreId();
   auto &active = gActiveDisplayList[core];

   if (active.buffer != displayList) {
      return 0;
   }

   auto bytes = active.curSize * 4;

   // Reset active dlist
   active.buffer = nullptr;
   active.curSize = 0;
   active.maxSize = 0;

   // Reset active command buffer
   gx2::internal::setUserCommandBuffer(nullptr);
   return bytes;
}

BOOL
GX2GetDisplayListWriteStatus()
{
   auto core = OSGetCoreId();
   auto &active = gActiveDisplayList[core];
   return active.buffer ? TRUE : FALSE;
}

BOOL
GX2GetCurrentDisplayList(be_ptr<void> *outDisplayList, be_val<uint32_t> *outSize)
{
   auto core = OSGetCoreId();
   auto &active = gActiveDisplayList[core];

   if (outDisplayList) {
      *outDisplayList = active.buffer;
   }

   if (outSize) {
      *outSize = active.maxSize;
   }

   return GX2GetDisplayListWriteStatus();
}

void
GX2DirectCallDisplayList(void *displayList, uint32_t size)
{
   GX2Flush();
   gpu::queueUserBuffer(displayList, size);
}

void
GX2CallDisplayList(void *displayList, uint32_t size)
{
   pm4::write(pm4::IndirectBufferCall { displayList, size / 4 });
}

void
GX2CopyDisplayList(void *displayList, uint32_t size)
{
   // Copy the display list to the current command buffer
   auto words = size / 4;
   auto dst = gx2::internal::getCommandBuffer(words);
   memcpy(&dst->buffer[dst->curSize], displayList, size);
   dst->curSize += words;
}
