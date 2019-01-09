#include "cpu.h"
#include <vector>

namespace cpu
{

constexpr auto MaxRegisteredKernelCalls = 0xffff; // This must be `(1<<Bits)-1` due to AND below.

Core * noopUnknownKcHandler(Core *core, uint32_t id)
{
   return core;
}

static KernelCallHandler sUnknownHandler = &noopUnknownKcHandler;
static std::atomic<KernelCallHandler> sHandlers[MaxRegisteredKernelCalls] = { nullptr };
static std::atomic_uint32_t sValidHandlerCount = 0;
static std::atomic_uint32_t sIllegalHandlerCount = 0;

void
setUnknownKernelCallHandler(KernelCallHandler handler)
{
   sUnknownHandler = handler;
}

uint32_t
registerKernelCallHandler(KernelCallHandler handler)
{
   uint32_t kcId = sValidHandlerCount++;
   sHandlers[kcId] = handler;
   return 0x100000 | kcId;
}

uint32_t
registerIllegalKernelCall()
{
   return 0x800000 | (++sIllegalHandlerCount);
}

KernelCallHandler
getKernelCallHandler(uint32_t id)
{
   if (LIKELY(id & 0x100000)) {
      return sHandlers[id & MaxRegisteredKernelCalls];
   }
   return sUnknownHandler;
}


} // namespace cpu
