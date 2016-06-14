#pragma once
#include "common/types.h"
#include "modules/coreinit/coreinit_time.h"
#include "virtual_ptr.h"

namespace pm4
{

struct Buffer
{
   bool userBuffer = false;
   coreinit::OSTime submitTime = 0;
   uint32_t *buffer = nullptr;
   uint32_t curSize = 0;
   uint32_t maxSize = 0;
};

Buffer *
getBuffer(uint32_t size);

Buffer *
flushBuffer(Buffer *buffer);

} // namespace pm4
