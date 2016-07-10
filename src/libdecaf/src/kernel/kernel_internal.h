#pragma once
#include "common/types.h"
#include "common/platform_fiber.h"
#include "modules/coreinit/coreinit_thread.h"

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
