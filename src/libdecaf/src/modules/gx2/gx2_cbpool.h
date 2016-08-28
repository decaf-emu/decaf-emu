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
initCommandBufferPool(virtual_ptr<uint32_t> base,
                      uint32_t size);

pm4::Buffer *
flushCommandBuffer(uint32_t neededSize);

void
freeCommandBuffer(pm4::Buffer *cb);

pm4::Buffer *
getCommandBuffer(uint32_t size);

void
padCommandBuffer(pm4::Buffer *buffer);

void
queueDisplayList(uint32_t *buffer,
                 uint32_t size);

bool
getUserCommandBuffer(uint32_t **buffer,
                     uint32_t *maxSize);

void
beginUserCommandBuffer(uint32_t *buffer,
                       uint32_t size);

uint32_t
endUserCommandBuffer(uint32_t *buffer);

} // namespace internal

} // namespace gx2
