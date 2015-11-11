#pragma once
#include "types.h"
#include "utils/virtual_ptr.h"

namespace pm4
{
struct CommandBuffer;
}

namespace gx2
{

namespace internal
{

void
initCommandBufferPool(virtual_ptr<uint32_t> base, uint32_t size, uint32_t itemSize);

pm4::CommandBuffer *
allocateCommandBuffer();

pm4::CommandBuffer *
flushCommandBuffer(pm4::CommandBuffer *cb);

pm4::CommandBuffer *
getCommandBuffer(uint32_t size);

} // namespace internal

} // namespace gx2
