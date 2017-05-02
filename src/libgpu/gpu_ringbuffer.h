#pragma once
#include <cstdint>

namespace gpu
{

namespace ringbuffer
{

struct Item
{
   void *context;
   uint32_t *buffer;
   uint32_t numWords;
};

void
submit(void *context,
       uint32_t *buffer,
       uint32_t numWords);

Item
dequeueItem();

Item
waitForItem();

void
awaken();

} // namespace ringbuffer

} // namespace gpu
