#pragma once
#include "common/types.h"
#include "modules/coreinit/coreinit_thread.h"

namespace kernel
{

void
initCoreFiber();

void
reallocateContextFiber(coreinit::OSContext *context, void(*fn)(void*));

void
saveContext(coreinit::OSContext *context);

void
restoreContext(coreinit::OSContext *context);

} // namespace kernel
