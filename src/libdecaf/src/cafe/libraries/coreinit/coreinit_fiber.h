#pragma once
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_fiber Fiber
 * \ingroup coreinit
 * @{
 */

using OSFiberEntryFn = virt_func_ptr<uint32_t()>;
using OSFiberExEntryFn = virt_func_ptr<uint32_t(uint32_t arg1,
                                                uint32_t arg2,
                                                uint32_t arg3,
                                                uint32_t arg4)>;

uint32_t
OSSwitchFiber(OSFiberEntryFn entry,
              virt_addr userStack);

uint32_t
OSSwitchFiberEx(uint32_t arg1,
                uint32_t arg2,
                uint32_t arg3,
                uint32_t arg4,
                OSFiberExEntryFn entry,
                virt_addr userStack);

/** @} */

} // namespace cafe::coreinit
