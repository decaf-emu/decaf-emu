#pragma once
#include "common/types.h"
#include "virtual_ptr.h"

namespace pm4
{
struct Buffer;
}

namespace gx2
{

namespace internal
{

void
initCommandBufferPool(virtual_ptr<uint32_t> base, uint32_t size, uint32_t itemSize);

pm4::Buffer *
flushCommandBuffer(pm4::Buffer *cb);

pm4::Buffer *
getCommandBuffer(uint32_t size);

void
setUserCommandBuffer(pm4::Buffer *userBuffer);

} // namespace internal

} // namespace gx2
