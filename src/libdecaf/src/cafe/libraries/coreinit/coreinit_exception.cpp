#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_exception.h"
#include "coreinit_interrupts.h"
#include "coreinit_thread.h"
#include "cafe/kernel/cafe_kernel_exception.h"

namespace cafe::coreinit
{

struct StaticExceptionData
{
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> dsiCallback;
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> isiCallback;
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> programCallback;
   be2_array<OSExceptionCallbackFn, OSGetCoreCount()> alignCallback;
};

static virt_ptr<StaticExceptionData>
sGlobalExceptions = nullptr;

static void
unkSyscall0x7A00(kernel::ExceptionType type,
                 uint32_t value)
{
   // TODO: I think this might be clear & enable or something?
}

OSExceptionCallbackFn
OSSetExceptionCallback(OSExceptionType type,
                       OSExceptionCallbackFn callback)
{
   return OSSetExceptionCallbackEx(OSExceptionMode::Thread, type, callback);
}

OSExceptionCallbackFn
OSSetExceptionCallbackEx(OSExceptionMode mode,
                         OSExceptionType type,
                         OSExceptionCallbackFn callback)
{
   // Only certain exceptions are supported for callback
   if (type != OSExceptionType::DSI &&
       type != OSExceptionType::ISI &&
       type != OSExceptionType::Alignment &&
       type != OSExceptionType::Program &&
       type != OSExceptionType::PerformanceMonitor) {
      return nullptr;
   }

   auto restoreInterruptValue = OSDisableInterrupts();
   auto previousCallback = internal::getExceptionCallback(mode, type);
   internal::setExceptionCallback(mode, type, callback);

   if (mode == OSExceptionMode::Thread &&
       type == OSExceptionType::PerformanceMonitor) {

   }

   OSRestoreInterrupts(restoreInterruptValue);
   return previousCallback;
}

namespace internal
{

OSExceptionCallbackFn
getExceptionCallback(OSExceptionMode mode,
                     OSExceptionType type)
{
   auto thread = OSGetCurrentThread();
   auto core = OSGetCoreId();
   auto callback = OSExceptionCallbackFn { nullptr };

   if (mode == OSExceptionMode::Thread ||
       mode == OSExceptionMode::ThreadAllCores ||
       mode == OSExceptionMode::System) {
      switch (type) {
      case OSExceptionType::DSI:
         callback = thread->dsiCallback[core];
         break;
      case OSExceptionType::ISI:
         callback = thread->isiCallback[core];
         break;
      case OSExceptionType::Alignment:
         callback = thread->alignCallback[core];
         break;
      case OSExceptionType::Program:
         callback = thread->programCallback[core];
         break;
      case OSExceptionType::PerformanceMonitor:
         callback = thread->perfMonCallback[core];
         break;
      }

      if (callback ||
          mode != OSExceptionMode::System) {
         return callback;
      }
   }

   if (mode == OSExceptionMode::Global ||
       mode == OSExceptionMode::GlobalAllCores ||
       mode == OSExceptionMode::System) {
      switch (type) {
      case OSExceptionType::DSI:
         callback = sGlobalExceptions->dsiCallback[core];
      case OSExceptionType::ISI:
         callback = sGlobalExceptions->isiCallback[core];
      case OSExceptionType::Alignment:
         callback = sGlobalExceptions->alignCallback[core];
      case OSExceptionType::Program:
         callback = sGlobalExceptions->programCallback[core];
      default:
         return nullptr;
      }
   }

   return callback;
}

void
setExceptionCallback(OSExceptionMode mode,
                     OSExceptionType type,
                     OSExceptionCallbackFn callback)
{
   auto thread = OSGetCurrentThread();
   auto core = OSGetCoreId();

   if (mode == OSExceptionMode::Thread) {
      switch (type) {
      case OSExceptionType::DSI:
         thread->dsiCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::DSI, 0);
         break;
      case OSExceptionType::ISI:
         thread->isiCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::ISI, 0);
         break;
      case OSExceptionType::Alignment:
         thread->alignCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Alignment, 0);
         break;
      case OSExceptionType::Program:
         thread->programCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Program, 0);
         break;
      case OSExceptionType::PerformanceMonitor:
         thread->perfMonCallback[core] = callback;
         break;
      }
   } else if (mode == OSExceptionMode::ThreadAllCores) {
      switch (type) {
      case OSExceptionType::DSI:
         thread->dsiCallback[0] = callback;
         thread->dsiCallback[1] = callback;
         thread->dsiCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::DSI, 0);
         break;
      case OSExceptionType::ISI:
         thread->isiCallback[0] = callback;
         thread->isiCallback[1] = callback;
         thread->isiCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::ISI, 0);
         break;
      case OSExceptionType::Alignment:
         thread->alignCallback[0] = callback;
         thread->alignCallback[1] = callback;
         thread->alignCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Alignment, 0);
         break;
      case OSExceptionType::Program:
         thread->programCallback[0] = callback;
         thread->programCallback[1] = callback;
         thread->programCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Program, 0);
         break;
      case OSExceptionType::PerformanceMonitor:
         thread->perfMonCallback[0] = callback;
         thread->perfMonCallback[1] = callback;
         thread->perfMonCallback[2] = callback;
         break;
      }
   } else if (mode == OSExceptionMode::Global) {
      switch (type) {
      case OSExceptionType::DSI:
         sGlobalExceptions->dsiCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::DSI, 0);
         break;
      case OSExceptionType::ISI:
         sGlobalExceptions->isiCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::ISI, 0);
         break;
      case OSExceptionType::Alignment:
         sGlobalExceptions->alignCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Alignment, 0);
         break;
      case OSExceptionType::Program:
         sGlobalExceptions->programCallback[core] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Program, 0);
         break;
      }
   } else if (mode == OSExceptionMode::GlobalAllCores) {
      switch (type) {
      case OSExceptionType::DSI:
         sGlobalExceptions->dsiCallback[0] = callback;
         sGlobalExceptions->dsiCallback[1] = callback;
         sGlobalExceptions->dsiCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::DSI, 0);
         break;
      case OSExceptionType::ISI:
         sGlobalExceptions->isiCallback[0] = callback;
         sGlobalExceptions->isiCallback[1] = callback;
         sGlobalExceptions->isiCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::ISI, 0);
         break;
      case OSExceptionType::Alignment:
         sGlobalExceptions->alignCallback[0] = callback;
         sGlobalExceptions->alignCallback[1] = callback;
         sGlobalExceptions->alignCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Alignment, 0);
         break;
      case OSExceptionType::Program:
         sGlobalExceptions->programCallback[0] = callback;
         sGlobalExceptions->programCallback[1] = callback;
         sGlobalExceptions->programCallback[2] = callback;
         unkSyscall0x7A00(kernel::ExceptionType::Program, 0);
         break;
      }
   }
}

void
initialiseExceptionHandlers()
{
   // TODO: initialiseExceptionHandlers
}

} // namespace internal

void
Library::registerExceptionSymbols()
{
   RegisterFunctionExport(OSSetExceptionCallback);
   RegisterFunctionExport(OSSetExceptionCallbackEx);
}

} // namespace cafe::coreinit
