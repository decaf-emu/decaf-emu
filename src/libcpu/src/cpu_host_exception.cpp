#include "cpu.h"

#include <common/decaf_assert.h>
#include <common/platform_exception.h>
#include <common/platform_stacktrace.h>
#include <common/platform_thread.h>
#include <fmt/format.h>

namespace cpu::internal
{

static thread_local uint32_t sSegfaultAddr = 0;
static thread_local platform::StackTrace *sSegfaultStackTrace = nullptr;
static SegfaultHandler sUserSegfaultHandler = nullptr;

static void
coreSegfaultEntry()
{
   auto core = cpu::this_core::state();
   if (sSegfaultAddr == core->nia) {
      core->srr0 = sSegfaultAddr;
   } else {
      core->srr0 = core->nia;
      core->dar = sSegfaultAddr;
      core->dsisr = 0u;
   }

   if (sUserSegfaultHandler) {
      sUserSegfaultHandler(core, sSegfaultAddr, sSegfaultStackTrace);
      decaf_abort("The user segfault handler unexpectedly returned.");
   } else {
      decaf_host_fault(fmt::format("Segfault exception, srr0: 0x{:08X}, dar: 0x{:08X}\n",
                                   core->srr0, core->dar),
                       sSegfaultStackTrace);
   }
}

static void
illegalInstructionHandler()
{
   auto core = cpu::this_core::state();
   core->srr0 = sSegfaultAddr;
   decaf_host_fault(fmt::format("Illegal instruction exception, srr0: 0x{:08X}\n",
                                core->srr0),
                    sSegfaultStackTrace);
}

static platform::ExceptionResumeFunc
hostExceptionHandler(platform::Exception *exception)
{
   // Handle illegal instructions!
   if (exception->type == platform::Exception::InvalidInstruction) {
      sSegfaultStackTrace = platform::captureStackTrace();
      return illegalInstructionHandler;
   }

   // Only handle AccessViolation exceptions
   if (exception->type != platform::Exception::AccessViolation) {
      return platform::UnhandledException;
   }

   // Only handle exceptions from the CPU cores
   if (this_core::id() >= 0xFF) {
      return platform::UnhandledException;
   }

   // Retreive the exception information
   auto info = reinterpret_cast<platform::AccessViolationException *>(exception);
   auto address = info->address;

   // Only handle exceptions within the virtual memory bounds
   auto memBase = getBaseVirtualAddress();
   if (address != 0 && (address < memBase || address >= memBase + 0x100000000)) {
      return platform::UnhandledException;
   }

   sSegfaultAddr = static_cast<uint32_t>(address - memBase);
   sSegfaultStackTrace = platform::captureStackTrace();
   return coreSegfaultEntry;
}

void
installHostExceptionHandler()
{
   static bool installed = false;
   if (!installed) {
      installed = platform::installExceptionHandler(hostExceptionHandler);
   }
}

void
setUserSegfaultHandler(SegfaultHandler userHandler)
{
   sUserSegfaultHandler = userHandler;
}

} // namespace cpu::internal
