#pragma once
#include <libcpu/be2_struct.h>
#include <string>

namespace cpu
{
struct Core;
}

namespace cafe::kernel
{

struct Context;

void
start();

void
join();

void
stop();

bool
hasExited();

void
setExecutableFilename(const std::string &name);

void
setSubCoreEntryContext(int coreId,
                       virt_ptr<Context> context);

namespace internal
{

void
idleCoreLoop(cpu::Core *core);

void
exit();

} // internal

} // namespace cafe::kernel
