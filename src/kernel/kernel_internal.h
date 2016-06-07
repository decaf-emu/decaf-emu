#pragma once
#include "types.h"
#include "modules/coreinit/coreinit_thread.h"

namespace kernel
{

void
saveContext(coreinit::OSContext *context);

void
restoreContext(coreinit::OSContext *context);

}