#pragma once
#include "cpu.h"

namespace cpu::internal
{

void
installHostExceptionHandler();

void
setUserSegfaultHandler(SegfaultHandler userHandler);

} // namespace cpu::internal
