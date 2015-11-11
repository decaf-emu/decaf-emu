#pragma once

namespace pm4
{
struct Buffer;
}

namespace gpu
{

void queueCommandBuffer(pm4::Buffer *buf);
void retireCommandBuffer(pm4::Buffer *buf);
pm4::Buffer *unqueueCommandBuffer();

}
