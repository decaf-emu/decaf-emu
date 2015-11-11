#pragma once
#include "types.h"
#include "modules/coreinit/coreinit_time.h"
#include "utils/virtual_ptr.h"

namespace pm4
{

struct Buffer
{
   OSTime submitTime;
   virtual_ptr<uint32_t> buffer;
   uint32_t curSize;
   uint32_t maxSize;
};

Buffer *
getBuffer(uint32_t size);

Buffer *
flushBuffer(Buffer *buffer);

} // namespace pm4
