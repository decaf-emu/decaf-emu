#include "gx2_cbpool.h"
#include "gx2_event.h"
#include "gx2_displaylist.h"
#include "gx2_state.h"
#include "gpu/pm4_buffer.h"
#include "gpu/commandqueue.h"
#include "modules/coreinit/coreinit_core.h"
#include "common/log.h"
#include "common/decaf_assert.h"
#include <tuple>
#include <vector>

struct CommandBufferPool
{
   uint32_t *base;
   uint32_t size;
   uint32_t itemSize = 0x100;
   uint32_t itemsHead = 0;
   std::vector<pm4::Buffer> items;
};

static CommandBufferPool
gCommandBufferPool;

static pm4::Buffer *
gActiveBuffer[coreinit::CoreCount] = { nullptr, nullptr, nullptr };

namespace gx2
{

namespace internal
{

static pm4::Buffer *
allocateCommandBuffer()
{
   if (gx2::internal::getMainCoreId() != coreinit::OSGetCoreId()) {
      // Only the main core can have command buffers
      gLog->warn("Tried to allocate command buffer on non-main graphics core");
      return nullptr;
   }

   auto retiredTimestamp = GX2GetRetiredTimeStamp();
   auto buffer = &gCommandBufferPool.items[gCommandBufferPool.itemsHead];

   if (buffer->submitTime > retiredTimestamp) {
      GX2WaitTimeStamp(buffer->submitTime);
   }

   buffer->userBuffer = false;
   buffer->submitTime = 0;
   buffer->curSize = 0;
   buffer->maxSize = gCommandBufferPool.itemSize / 4;
   buffer->buffer = gCommandBufferPool.base + buffer->maxSize * gCommandBufferPool.itemsHead;
   gCommandBufferPool.itemsHead++;

   if (gCommandBufferPool.itemsHead >= gCommandBufferPool.items.size()) {
      gCommandBufferPool.itemsHead = 0;
   }

   return buffer;
}

void
initCommandBufferPool(virtual_ptr<uint32_t> base, uint32_t size, uint32_t itemSize)
{
   auto core = coreinit::OSGetCoreId();
   decaf_check(gx2::internal::getMainCoreId() == core);

   gCommandBufferPool.base = base;
   gCommandBufferPool.size = size;
   gCommandBufferPool.itemSize = itemSize;
   gCommandBufferPool.items.resize(size / gCommandBufferPool.itemSize);
   gActiveBuffer[core] = allocateCommandBuffer();
}

pm4::Buffer *
flushCommandBuffer(pm4::Buffer *cb)
{
   auto core = coreinit::OSGetCoreId();

   if (!cb) {
      cb = gActiveBuffer[core];
   } else if (cb != gActiveBuffer[core]) {
      decaf_abort("Attempting to flush non-active buffer");
   }

   if (!cb) {
      return nullptr;
   }

   if (cb->displayList) {
      void *newList = nullptr;
      uint32_t newSize = 0;

      // End the active display list
      GX2EndDisplayList(cb->buffer);

      // Ask user to allocate new display list
      std::tie(newList, newSize) = displayListOverrun(cb->buffer, cb->curSize * 4);

      if (!newList || !newSize) {
         decaf_abort("Unable to handle display list overrun");
      }

      // Begin new display list, it will update gActiveBuffer
      GX2BeginDisplayList(newList, newSize);
   } else {
      // Send buffer to our driver!
      gpu::queueCommandBuffer(cb);

      // Allocate new buffer
      gActiveBuffer[core] = allocateCommandBuffer();
   }

   return gActiveBuffer[core];
}

pm4::Buffer *
getCommandBuffer(uint32_t size)
{
   auto core = coreinit::OSGetCoreId();
   auto &active = gActiveBuffer[core];

   if (!active) {
      active = allocateCommandBuffer();
   } else if (active->curSize + size > active->maxSize) {
      active = flushCommandBuffer(active);
   }

   return active;
}

void
setUserCommandBuffer(pm4::Buffer *buffer)
{
   auto core = coreinit::OSGetCoreId();

   if (buffer) {
      buffer->userBuffer = true;
   }

   // Flush current active buffer
   auto active = gActiveBuffer[core];

   if (active && !active->displayList) {
      gpu::queueCommandBuffer(active);
   }

   // Set new active buffer to user command buffer
   gActiveBuffer[core] = buffer;
}

} // namespace internal

} // namespace gx2
