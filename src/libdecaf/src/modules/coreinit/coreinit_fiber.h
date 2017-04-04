#pragma once
#include "ppcutils/wfunc_ptr.h"
#include <cstdint>

namespace coreinit
{

using OSFiberEntryFn = wfunc_ptr<void>;
using OSFiberExEntryFn = wfunc_ptr<void, uint32_t, uint32_t, uint32_t, uint32_t>;

void
OSSwitchFiber(OSFiberEntryFn fn,
              uint32_t userStack);

void
OSSwitchFiberEx(uint32_t r3,
                uint32_t r4,
                uint32_t r5,
                uint32_t r6,
                OSFiberExEntryFn fn,
                uint32_t userStack);

} // namespace coreinit
