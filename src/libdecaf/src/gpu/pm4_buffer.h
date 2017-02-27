#pragma once
#include "modules/coreinit/coreinit_time.h"

#include <atomic>
#include <cstdint>

namespace pm4
{

struct Buffer
{
   bool displayList = false;
   coreinit::OSTime submitTime = 0;
   uint32_t *buffer = nullptr;
   uint32_t curSize = 0;
   uint32_t maxSize = 0;

   std::atomic<Buffer *> next;
};

Buffer *
getBuffer(uint32_t size);

} // namespace pm4
