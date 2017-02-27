#pragma once
#include "modules/coreinit/coreinit_thread.h"

#include <common/platform_fiber.h>

namespace kernel
{

void
initCoreFiber();

void
reallocateContextFiber(coreinit::OSContext *context,
                       platform::FiberEntryPoint entry);

void
saveContext(coreinit::OSContext *context);

void
restoreContext(coreinit::OSContext *context);

} // namespace kernel
