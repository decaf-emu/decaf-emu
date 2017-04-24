#pragma once
#include <libcpu/pointer.h>

namespace gpu
{

namespace ringbuffer
{

struct Item
{
   void *context;
   cpu::VirtualPointer<uint32_t> buffer;
   uint32_t numWords;
};

void
submit(void *context,
       cpu::VirtualPointer<uint32_t> buffer,
       uint32_t numWords);

Item
dequeueItem();

Item
waitForItem();

void
awaken();

} // namespace ringbuffer

} // namespace gpu
