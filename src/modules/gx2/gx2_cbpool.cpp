#include <cassert>
#include <vector>
#include "gx2_cbpool.h"
#include "gx2_event.h"
#include "gpu/pm4_buffer.h"

struct CommandBufferPool
{
   uint32_t *base;
   uint32_t size;
   uint32_t itemSize = 0x100;
   uint32_t itemsHead = 0;
   std::vector<pm4::Buffer> items;
};

CommandBufferPool gCommandBufferPool;
pm4::Buffer *gActiveBuffer = nullptr;

namespace gx2
{

namespace internal
{

void
initCommandBufferPool(virtual_ptr<uint32_t> base, uint32_t size, uint32_t itemSize)
{
   gCommandBufferPool.base = base;
   gCommandBufferPool.size = size;
   gCommandBufferPool.itemSize = itemSize;
   gCommandBufferPool.items.resize(size / gCommandBufferPool.itemSize);
   allocateCommandBuffer();
}

pm4::Buffer *
allocateCommandBuffer()
{
   OSTime retiredTimestamp = GX2GetRetiredTimeStamp();
   auto buffer = &gCommandBufferPool.items[gCommandBufferPool.itemsHead];

   if (buffer->submitTime > retiredTimestamp) {
      GX2WaitTimeStamp(buffer->submitTime);
   }

   buffer->submitTime = 0;
   buffer->curSize = 0;
   buffer->maxSize = gCommandBufferPool.itemSize / 4;
   buffer->buffer = gCommandBufferPool.base + buffer->maxSize * gCommandBufferPool.itemsHead;
   gCommandBufferPool.itemsHead++;

   if (gCommandBufferPool.itemsHead >= gCommandBufferPool.items.size()) {
      gCommandBufferPool.itemsHead = 0;
   }

   gActiveBuffer = buffer;
   return buffer;
}

pm4::Buffer *
flushCommandBuffer(pm4::Buffer *cb)
{
   assert(cb == gActiveBuffer);

   // TODO: Flush buffer to GX2 driver backened
   cb->submitTime = OSGetTime();

   // Allocate and return new command buffer
   return allocateCommandBuffer();
}

pm4::Buffer *
getCommandBuffer(uint32_t size)
{
   auto buffer = gActiveBuffer;

   if (buffer->curSize + size > buffer->maxSize) {
      buffer = flushCommandBuffer(buffer);
   }

   return buffer;
}

}

}