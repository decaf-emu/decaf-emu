#pragma once
#include "common/types.h"

namespace pm4
{
struct Buffer;
}

namespace gpu
{

void
awaken();

void
queueCommandBuffer(pm4::Buffer *buf);

void
retireCommandBuffer(pm4::Buffer *buf);

pm4::Buffer *
unqueueCommandBuffer();

pm4::Buffer *
tryUnqueueCommandBuffer();

} // namespace gpu
