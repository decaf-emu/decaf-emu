#pragma once
#include "common/types.h"
#include "modules/coreinit/coreinit_time.h"
#include "virtual_ptr.h"
#include <atomic>

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
