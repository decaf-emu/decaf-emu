#include "cafe_kernel_context.h"
#include "cafe_kernel_exception.h"
#include "cafe_kernel_interrupts.h"
#include "cafe_kernel_heap.h"
#include "cafe_kernel_ipckdriver.h"
#include "cafe/cafe_ppc_interface_invoke.h"

#include "decaf_config.h"
#include "debug_api/debug_api_controller.h"
#include "cafe/libraries/coreinit/coreinit_alarm.h"
#include "cafe/libraries/coreinit/coreinit_interrupts.h"
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/gx2/gx2_event.h"

#include <array>
#include <common/log.h>
#include <common/platform_stacktrace.h>
#include <common/platform_thread.h>
#include <libcpu/cpu.h>
#include <vector>
#include <queue>

namespace cafe::kernel
{

constexpr auto ExceptionThreadStackSize = 0x100u;

struct StaticExceptionData
{
   be2_array<Context, 3> exceptionThreadContext;
   be2_array<std::byte, ExceptionThreadStackSize * 3> exceptionStackBuffer;
};

static virt_ptr<StaticExceptionData>
sExceptionData;

static std::array<ExceptionHandlerFn, ExceptionType::Max>
sUserExceptionHandlers;

static std::array<ExceptionHandlerFn, ExceptionType::Max>
sKernelExceptionHandlers;

static std::array<platform::StackTrace *, 3>
sExceptionStackTraces;

bool
setUserModeExceptionHandler(ExceptionType type,
                            ExceptionHandlerFn handler)
{
   if (sUserExceptionHandlers[type]) {
      return false;
   }

   sUserExceptionHandlers[type] = handler;
   return true;
}

namespace internal
{

static void
defaultExceptionHandler(ExceptionType type,
                        virt_ptr<Context> interruptedContext);

inline void
dispatchException(ExceptionType type,
                  virt_ptr<Context> interruptedContext)
{
   if (sKernelExceptionHandlers[type]) {
      sKernelExceptionHandlers[type](type, interruptedContext);
   } else if (sUserExceptionHandlers[type]) {
      sUserExceptionHandlers[type](type, interruptedContext);
   } else {
      defaultExceptionHandler(type, interruptedContext);
   }
}

static void
exceptionContextFiberEntry(void *)
{
   auto coreId = cpu::this_core::id();
   wakeCurrentContext();

   while (true) {
      auto context = getCurrentContext();
      auto interruptedContext = virt_cast<Context *>(virt_addr { context->gpr[3].value() });
      auto flags = context->gpr[4];

      if (flags & cpu::ALARM_INTERRUPT) {
         dispatchException(ExceptionType::Decrementer, interruptedContext);
      }

      if (flags & cpu::GPU7_INTERRUPT) {
         dispatchExternalInterrupt(InterruptType::Gpu7, interruptedContext);
      }

      if (flags & cpu::IPC_INTERRUPT) {
         dispatchExternalInterrupt(static_cast<InterruptType>(InterruptType::IpcPpc0 + coreId),
                                   interruptedContext);
      }

      // Return to interrupted context
      switchContext(interruptedContext);
   }
}

static void
handleCpuInterrupt(cpu::Core *core,
                   uint32_t flags)
{
   auto interruptedContext = getCurrentContext();

   if (flags & cpu::SRESET_INTERRUPT) {
      dispatchException(ExceptionType::SystemReset, interruptedContext);
   }

   if (flags & cpu::DBGBREAK_INTERRUPT) {
      dispatchException(ExceptionType::Breakpoint, interruptedContext);
   }

   // Disable interrupts
   auto originalInterruptMask =
      cpu::this_core::setInterruptMask(cpu::SRESET_INTERRUPT |
                                       cpu::DBGBREAK_INTERRUPT);

   // Switch to the exception context fiber
   auto exceptionContext = virt_addrof(sExceptionData->exceptionThreadContext[core->id]);
   exceptionContext->gpr[3] = static_cast<uint32_t>(virt_cast<virt_addr>(interruptedContext));
   exceptionContext->gpr[4] = flags;
   switchContext(exceptionContext);

   // Restore interrupts
   cpu::this_core::setInterruptMask(originalInterruptMask);

   // Always dispatch an ICI so userland coreinit can reschedule
   dispatchException(ExceptionType::ICI, interruptedContext);
}

struct UnhandledExceptionData
{
   ExceptionType type;
   virt_ptr<Context> context;
   int coreId;
   platform::StackTrace *stackTrace = nullptr;
};

static void
unhandledExceptionFiberEntryPoint(void *param)
{
   auto exceptionData = reinterpret_cast<UnhandledExceptionData *>(param);
   auto context = exceptionData->context;

   // We may have been in the middle of a kernel function...
   if (coreinit::internal::isSchedulerLocked()) {
      coreinit::internal::unlockScheduler();
   }

   // Log the core state
   fmt::memory_buffer out;
   fmt::format_to(out, "Unhandled exception {}\n", exceptionData->type);
   fmt::format_to(out, "Warning: Register values may not be reliable when using JIT.\n");

   switch (exceptionData->type) {
   case ExceptionType::DSI:
      fmt::format_to(out, "Core{} Instruction at 0x{:08X} (value from SRR0) attempted to access invalid address 0x{:08X} (value from DAR)\n",
                     exceptionData->coreId, context->srr0, context->dar);
      break;
   case ExceptionType::ISI:
      fmt::format_to(out, "Core{} Attempted to fetch instruction from invalid address 0x{:08X} (value from SRR0)\n",
                     exceptionData->coreId, context->srr0);
      break;
   case ExceptionType::Alignment:
      fmt::format_to(out, "Core{} Instruction at 0x{:08X} (value from SRR0) attempted to access unaligned address 0x{:08X} (value from DAR)\n",
                     exceptionData->coreId, context->srr0, context->dar);
      break;
   case ExceptionType::Program:
      fmt::format_to(out, "Core{} Program exception: Possible illegal instruction/operation at or around 0x{:08X} (value from SRR0)\n",
                     exceptionData->coreId, context->srr0);
      break;
   default:
      break;
   }

   fmt::format_to(out, "nia: 0x{:08x}\n", context->nia);
   fmt::format_to(out, "lr: 0x{:08x}\n", context->lr);
   fmt::format_to(out, "cr: 0x{:08x}\n", context->cr);
   fmt::format_to(out, "ctr: 0x{:08x}\n", context->ctr);
   fmt::format_to(out, "xer: 0x{:08x}\n", context->xer);

   for (auto i = 0u; i < 32; ++i) {
      fmt::format_to(out, "gpr[{}]: 0x{:08x}\n", i, context->gpr[i]);
   }

   gLog->critical(std::string_view { out.data(), out.size() });

   // If the decaf debugger is enabled, we will catch this exception there
   if (decaf::config::debugger::enabled) {
      // Move back an instruction so we can re-execute the failed instruction
      //  and so that the debugger shows the right stop point.
      cpu::this_core::state()->nia -= 4;

      coreinit::internal::pauseCoreTime(true);
      decaf::debug::handleDebugBreakInterrupt();
      coreinit::internal::pauseCoreTime(false);

      // This will shut down the thread and reschedule.  This is required
      //  since returning from the segfault handler is an error.
      coreinit::OSExitThread(0);
   } else {
      // If there is no debugger then let's crash decaf lul!
      decaf_host_fault(fmt::format("Unhandled exception {}, srr0: 0x{:08X} nia: 0x{:08X}\n",
                                   exceptionData->type, context->srr0, context->nia),
                       exceptionData->stackTrace);
   }
}

static void
defaultExceptionHandler(ExceptionType type,
                        virt_ptr<Context> interruptedContext)
{
   auto exceptionData = new UnhandledExceptionData { };
   exceptionData->type = type;
   exceptionData->coreId = cpu::this_core::id();
   exceptionData->context = interruptedContext;
   exceptionData->stackTrace = sExceptionStackTraces[exceptionData->coreId];
   resetFaultedContextFiber(getCurrentContext(), unhandledExceptionFiberEntryPoint, exceptionData);
}

static void
handleCpuSegfault(cpu::Core *core,
                  uint32_t address,
                  platform::StackTrace *stackTrace)
{
   auto interruptedContext = getCurrentContext();
   copyContextFromCpu(interruptedContext);
   sExceptionStackTraces[core->id] = stackTrace;

   if (address == core->nia) {
      dispatchException(ExceptionType::ISI, interruptedContext);
   } else {
      dispatchException(ExceptionType::DSI, interruptedContext);
   }
}

static void
handleCpuIllegalInstruction(cpu::Core *core,
                            platform::StackTrace *stackTrace)
{
   auto interruptedContext = getCurrentContext();
   copyContextFromCpu(interruptedContext);
   sExceptionStackTraces[core->id] = stackTrace;
   dispatchException(ExceptionType::Program, interruptedContext);
}

static void
handleDebugBreakException(ExceptionType type,
                          virt_ptr<Context> interruptedContext)
{
   if (decaf::config::debugger::enabled) {
      coreinit::internal::pauseCoreTime(true);
      decaf::debug::handleDebugBreakInterrupt();
      coreinit::internal::pauseCoreTime(false);
   }
}

static void
handleIciException(ExceptionType type,
                   virt_ptr<Context> interruptedContext)
{
   // Call user ICI handler if set, else just ignore
   if (sUserExceptionHandlers[type]) {
      sUserExceptionHandlers[type](type, interruptedContext);
   }
}

static void
handleSystemResetException(ExceptionType type,
                           virt_ptr<Context> interruptedContext)
{
   platform::exitThread(0);
}

void
initialiseExceptionContext(cpu::Core *core)
{
   auto context = virt_addrof(sExceptionData->exceptionThreadContext[core->id]);
   std::memset(context.get(), 0, sizeof(Context));

   auto stack = virt_addrof(sExceptionData->exceptionStackBuffer[core->id * ExceptionThreadStackSize]);
   context->gpr[1] = virt_cast<virt_addr>(stack).getAddress() + ExceptionThreadStackSize - 8;
   context->attr |= 1 << core->id;

   setContextFiberEntry(context, exceptionContextFiberEntry, nullptr);
}

void
initialiseExceptionHandlers()
{
   setKernelExceptionHandler(ExceptionType::SystemReset,
                             handleSystemResetException);

   setKernelExceptionHandler(ExceptionType::Breakpoint,
                             handleDebugBreakException);

   setKernelExceptionHandler(ExceptionType::ICI,
                             handleIciException);

   // TODO: Move this to kernel timers
   setKernelExceptionHandler(ExceptionType::Decrementer,
      [](ExceptionType type,
         virt_ptr<Context> interruptedContext)
      {
         coreinit::internal::disableScheduler();
         coreinit::internal::handleAlarmInterrupt(interruptedContext);
         coreinit::internal::enableScheduler();
      });

   cpu::setInterruptHandler(&handleCpuInterrupt);
   cpu::setSegfaultHandler(&handleCpuSegfault);
   cpu::setIllInstHandler(&handleCpuIllegalInstruction);
}

void
initialiseStaticExceptionData()
{
   sExceptionData = allocStaticData<StaticExceptionData>();
}

void
setKernelExceptionHandler(ExceptionType type,
                          ExceptionHandlerFn handler)
{
   sKernelExceptionHandlers[type] = handler;
}

} // namespace internal

} // namespace cafe::kernel
