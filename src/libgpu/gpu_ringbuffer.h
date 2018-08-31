#pragma once
#include <cstdint>
#include <libcpu/be2_struct.h>

namespace gpu
{

namespace ringbuffer
{

struct Item
{
   void *context;
   phys_ptr<uint32_t> buffer;
   uint32_t numWords;
};

void
submit(void *context,
       phys_ptr<uint32_t> buffer,
       uint32_t numWords);

Item
dequeueItem();

Item
waitForItem();

void
awaken();

} // namespace ringbuffer

} // namespace gpu
