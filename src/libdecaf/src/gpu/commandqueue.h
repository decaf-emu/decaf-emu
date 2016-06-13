#pragma once
#include "common/types.h"

namespace pm4
{
struct Buffer;
}

namespace gpu
{

void
queueUserBuffer(void *buf, uint32_t bytes);

void
queueCommandBuffer(pm4::Buffer *buf);

void
retireCommandBuffer(pm4::Buffer *buf);

pm4::Buffer *
unqueueCommandBuffer();

} // namespace gpu
