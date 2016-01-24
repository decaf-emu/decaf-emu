#include "gx2_cbpool.h"
#include "gx2_displaylist.h"
#include "gx2_state.h"
#include "gpu/commandqueue.h"
#include "gpu/pm4_writer.h"
#include "modules/coreinit/coreinit_core.h"
#include "utils/align.h"
#include <array>

static std::array<pm4::Buffer, CoreCount>
gActiveDisplayList;

void
GX2BeginDisplayList(void *displayList, uint32_t bytes)
{
   GX2BeginDisplayListEx(displayList, bytes, TRUE);
}

void
GX2BeginDisplayListEx(void *displayList, uint32_t bytes, BOOL unk1)
{
   auto core = OSGetCoreId();
   auto &active = gActiveDisplayList[core];

   // Set active display list
   active.buffer = reinterpret_cast<uint32_t*>(displayList);
   active.curSize = 0;
   active.maxSize = bytes / 4;

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

   // Display list is meant to be padded to 32 bytes
   auto bytes = align_up(active.curSize * 4, 32);

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
GX2DirectCallDisplayList(void *displayList, uint32_t bytes)
{
   GX2Flush();
   gpu::queueUserBuffer(displayList, bytes);
}

void
GX2CallDisplayList(void *displayList, uint32_t bytes)
{
   pm4::write(pm4::IndirectBufferCall { displayList, bytes / 4 });
}

void
GX2CopyDisplayList(void *displayList, uint32_t bytes)
{
   // Copy the display list to the current command buffer
   auto words = bytes / 4;
   auto dst = gx2::internal::getCommandBuffer(words);
   memcpy(&dst->buffer[dst->curSize], displayList, bytes);
   dst->curSize += words;
}
