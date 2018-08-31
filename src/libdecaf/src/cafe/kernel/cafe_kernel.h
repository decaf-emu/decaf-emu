#pragma once
#include <string>

namespace cpu
{
struct Core;
}

namespace cafe::kernel
{

void
start();

void
join();

bool
hasExited();

void
setExecutableFilename(const std::string &name);

namespace internal
{

void
idleCoreLoop(cpu::Core *core);

void
exit();

} // internal

} // namespace cafe::kernel
