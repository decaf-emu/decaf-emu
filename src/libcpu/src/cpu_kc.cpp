#include "cpu.h"
#include <vector>

namespace cpu
{

static KernelCallHandler sHandler = nullptr;

void
setKernelCallHandler(KernelCallHandler handler)
{
   sHandler = handler;
}

void
onKernelCall(cpu::Core *core,
             uint32_t id)
{
   if (sHandler) {
      sHandler(core, id);
   }
}

} // namespace cpu
