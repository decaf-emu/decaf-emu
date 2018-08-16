#pragma once
#include "cafe/libraries/coreinit/coreinit_scheduler.h"
#include "cafe/libraries/coreinit/coreinit_thread.h"
#include "debugger_interface.h"

namespace debugger
{

static uint32_t
ThreadNotRunning = -1;

inline uint32_t
getThreadCoreId(virt_ptr<cafe::coreinit::OSThread> thread)
{
   if (thread) {
      for (auto i = 0u; i < 3; ++i) {
         if (thread == cafe::coreinit::internal::getCoreRunningThread(i)) {
            return i;
         }
      }
   }

   return ThreadNotRunning;
}

inline uint32_t
getCoreIdByThreadId(uint32_t id)
{
   for (auto i = 0u; i < 3; ++i) {
      auto thread = cafe::coreinit::internal::getCoreRunningThread(i);

      if (thread && thread->id == id) {
         return i;
      }
   }

   return ThreadNotRunning;
}

inline cpu::CoreRegs *
getThreadCoreContext(DebuggerInterface *debugger,
                     virt_ptr<cafe::coreinit::OSThread> thread)
{
   auto core = getThreadCoreId(thread);

   if (core == ThreadNotRunning) {
      return nullptr;
   }

   return debugger->getPauseContext(core);
}

inline uint32_t
getThreadNia(DebuggerInterface *debugger,
             virt_ptr<cafe::coreinit::OSThread> thread)
{
   auto context = getThreadCoreContext(debugger, thread);

   if (context) {
      return context->nia;
   } else if (thread) {
      return thread->context.nia;
   } else {
      return 0;
   }
}

inline uint32_t
getThreadStack(DebuggerInterface *debugger,
               virt_ptr<cafe::coreinit::OSThread> thread)
{
   auto context = getThreadCoreContext(debugger, thread);

   if (context) {
      return context->gpr[1];
   } else if (thread) {
      return thread->context.gpr[1];
   } else {
      return 0;
   }
}

inline virt_ptr<cafe::coreinit::OSThread>
getThreadById(uint32_t id)
{
   cafe::coreinit::internal::lockScheduler();
   auto firstThread = cafe::coreinit::internal::getFirstActiveThread();

   for (auto thread = firstThread; thread; thread = thread->activeLink.next) {
      if (thread->id == id) {
         cafe::coreinit::internal::unlockScheduler();
         return thread;
      }
   }

   cafe::coreinit::internal::unlockScheduler();
   return nullptr;
}

} // namespace debugger
