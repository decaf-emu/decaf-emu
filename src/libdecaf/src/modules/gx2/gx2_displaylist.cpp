#include "gx2_cbpool.h"
#include "gx2_displaylist.h"
#include "gx2_state.h"
#include "gpu/gpu_commandqueue.h"
#include "gpu/pm4_writer.h"
#include "modules/coreinit/coreinit_core.h"
#include "common/align.h"
#include "common/decaf_assert.h"
#include <array>

namespace gx2
{

void
GX2BeginDisplayList(void *displayList,
                    uint32_t bytes)
{
   GX2BeginDisplayListEx(displayList, bytes, TRUE);
}

void
GX2BeginDisplayListEx(void *displayList,
                      uint32_t bytes,
                      BOOL unk1)
{
   internal::beginUserCommandBuffer(reinterpret_cast<uint32_t *>(displayList), bytes / 4);
}

uint32_t
GX2EndDisplayList(void *displayList)
{
   return internal::endUserCommandBuffer(reinterpret_cast<uint32_t*>(displayList)) * 4;
}

BOOL
GX2GetDisplayListWriteStatus()
{
   return internal::getUserCommandBuffer(nullptr, nullptr) ? TRUE : FALSE;
}

BOOL
GX2GetCurrentDisplayList(be_ptr<void> *outDisplayList,
                         be_val<uint32_t> *outSize)
{
   uint32_t *displayList = nullptr;
   uint32_t size = 0;

   if (!internal::getUserCommandBuffer(&displayList, &size)) {
      return FALSE;
   }

   if (outDisplayList) {
      *outDisplayList = displayList;
   }

   if (outSize) {
      *outSize = size;
   }

   return TRUE;
}

void
GX2DirectCallDisplayList(void *displayList,
                         uint32_t bytes)
{
   GX2Flush();
   internal::queueDisplayList(reinterpret_cast<uint32_t*>(displayList), bytes / 4);
}

void
GX2CallDisplayList(void *displayList,
                   uint32_t bytes)
{
   pm4::write(pm4::IndirectBufferCall { displayList, bytes / 4 });
}

void
GX2CopyDisplayList(void *displayList,
                   uint32_t bytes)
{
   // Copy the display list to the current command buffer
   auto words = bytes / 4;
   auto dst = gx2::internal::getCommandBuffer(words);
   memcpy(&dst->buffer[dst->curSize], displayList, bytes);
   dst->curSize += words;
}

} // namespace gx2
