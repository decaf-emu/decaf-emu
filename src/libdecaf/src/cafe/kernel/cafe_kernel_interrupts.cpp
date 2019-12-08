#include "cafe_kernel_context.h"
#include "cafe_kernel_interrupts.h"
#include "cafe_kernel_process.h"

#include "cafe/cafe_ppc_interface_invoke_guest.h"

namespace cafe::kernel
{

struct InterruptData
{
   std::array<internal::KernelInterruptHandlerFn, InterruptType::Max> kernelHandlers;
   std::array<UserInterruptHandlerFn, InterruptType::Max> userHandlers;
   std::array<virt_ptr<void>, InterruptType::Max> userHandlerData;
};

static std::array<InterruptData, 3>
sPerCoreInterruptData;

UserInterruptHandlerFn
setUserModeInterruptHandler(InterruptType type,
                            UserInterruptHandlerFn handler,
                            virt_ptr<void> userData)
{
   auto &data = sPerCoreInterruptData[cpu::this_core::id()];
   if (type >= InterruptType::Max) {
      // Invalid interrupt type
      return nullptr;
   }

   auto previous = data.userHandlers[type];
   data.userHandlers[type] = handler;
   data.userHandlerData[type] = userData;
   return previous;
}

void
clearAndEnableInterrupt(InterruptType type)
{
}

void
disableInterrupt(InterruptType type)
{
}

namespace internal
{

void
dispatchExternalInterrupt(InterruptType type,
                          virt_ptr<Context> interruptedContext)
{
   auto &data = sPerCoreInterruptData[cpu::this_core::id()];

   if (auto kernelHandler = data.kernelHandlers[type]) {
      kernelHandler(type, interruptedContext);
   } else if (auto userHandler = data.userHandlers[type]) {
      userHandler(type, interruptedContext, data.userHandlerData[type]);
   }
}

void
setKernelInterruptHandler(InterruptType type,
                          KernelInterruptHandlerFn handler)
{
   sPerCoreInterruptData[cpu::this_core::id()].kernelHandlers[type] = handler;
}

} // namespace internal

} // namespace cafe::kernel
