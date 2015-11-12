#pragma once
#include "types.h"
#include "modules/coreinit/coreinit_time.h"
#include "utils/virtual_ptr.h"

namespace pm4
{

struct Buffer
{
   bool userBuffer = false;
   OSTime submitTime = 0;
   uint32_t *buffer = nullptr;
   uint32_t curSize = 0;
   uint32_t maxSize = 0;
};

Buffer *
getBuffer(uint32_t size);

Buffer *
flushBuffer(Buffer *buffer);

} // namespace pm4
