#include "cafe_kernel_context.h"
#include "cafe_kernel_interrupts.h"
#include "cafe_kernel_process.h"

#include "cafe/cafe_ppc_interface_invoke.h"

namespace cafe::kernel
{

struct InterruptData
{
   std::array<InterruptHandlerFn, InterruptType::Max> kernelHandlers;
   std::array<InterruptHandlerFn, InterruptType::Max> userHandlers;
};

static std::array<InterruptData, 3>
sPerCoreInterruptData;

InterruptHandlerFn
setUserModeInterruptHandler(InterruptType type,
                            InterruptHandlerFn handler)
{
   auto &data = sPerCoreInterruptData[cpu::this_core::id()];
   if (type >= InterruptType::Max) {
      // Invalid interrupt type
      return nullptr;
   }

   auto previous = data.userHandlers[type];
   data.userHandlers[type] = handler;
   return previous;
}

namespace internal
{

void
dispatchExternalInterrupt(InterruptType type,
                          virt_ptr<Context> interruptedContext)
{
   auto &data = sPerCoreInterruptData[cpu::this_core::id()];

   if (auto handler = data.kernelHandlers[type]) {
      handler(type, interruptedContext);
   } else if (auto handler = data.userHandlers[type]) {
      handler(type, interruptedContext);
   }
}

void
setKernelInterruptHandler(InterruptType type,
                          InterruptHandlerFn handler)
{
   sPerCoreInterruptData[cpu::this_core::id()].kernelHandlers[type] = handler;
}

} // namespace internal

} // namespace cafe::kernel
