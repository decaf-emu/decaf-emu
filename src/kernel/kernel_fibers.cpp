#include "kernel.h"
#include <algorithm>
#include <cfenv>
#include "cpu/cpu.h"
#include "cpu/mem.h"
#include "platform/platform_fiber.h"
#include "platform/platform_thread.h"
#include "modules/coreinit/coreinit.h"
#include "modules/coreinit/coreinit_core.h"
#include "modules/coreinit/coreinit_memheap.h"
#include "modules/coreinit/coreinit_thread.h"
#include "modules/coreinit/coreinit_scheduler.h"
#include "modules/coreinit/coreinit_systeminfo.h"

namespace kernel
{

static coreinit::OSThread *tDeadThread[3];
static platform::Fiber *tIdleFiber[3];

struct Fiber
{
   platform::Fiber *handle = nullptr;
   coreinit::OSThread *thread = nullptr;
   cpu::Tracer *tracer = nullptr;
};

static void
checkDeadThread();

static void
fiberEntryPoint(void*)
{
   checkDeadThread();

   // Our scheduler will have been locked by whoever
   // scheduled this fiber.
   coreinit::internal::unlockScheduler();

   auto core = cpu::this_core::state();
   // TODO: Do not use coreinit from the kernel...
   auto thread = coreinit::internal::getCurrentThread();

   auto entryPoint = coreinit::OSThreadEntryPointFn(core->lr);
   auto argc = core->gpr[3];
   auto argv = mem::translate<void>(core->gpr[4]);
   coreinit::OSExitThread(entryPoint(argc, argv));
}

static Fiber *
allocateFiber(coreinit::OSThread *thread)
{
   auto fiber = new Fiber();
   fiber->tracer = cpu::alloc_tracer(1024);
   fiber->handle = platform::createFiber(fiberEntryPoint, nullptr);
   fiber->thread = thread;
   return fiber;
}

static void
freeFiber(Fiber *fiber)
{
   cpu::free_tracer(fiber->tracer);
   platform::destroyFiber(fiber->handle);
}

// This must be called under the same scheduler lock
// that added the thread to tDeadThread, we simply use
// the thread_local to pass it between fibers.
static void
checkDeadThread()
{
   auto coreId = cpu::this_core::id();
   auto deadThread = tDeadThread[coreId];

   if (deadThread) {
      tDeadThread[coreId] = nullptr;

      // Something is broken if we have no fiber
      assert(deadThread->fiber);

      // Destroy the fiber
      freeFiber(deadThread->fiber);
   }
}

void init_core_fiber()
{
   // Grab the currently running core state.
   auto core_id = cpu::this_core::id();

   // Grab the system fiber
   auto fiber = platform::getThreadFiber();

   // Save some needed information about the fiber run states.
   tIdleFiber[core_id] = fiber;
   tDeadThread[core_id] = nullptr;
}

void
exitThreadNoLock()
{
   // Mark this fiber to be cleaned up
   tDeadThread[cpu::this_core::id()] = coreinit::internal::getCurrentThread();
}

void
saveContext(coreinit::OSContext *context)
{
   auto state = cpu::this_core::state();

   for (auto i = 0; i < 32; ++i) {
      context->gpr[i] = state->gpr[i];
   }

   for (auto i = 0; i < 32; ++i) {
      context->fpr[i] = state->fpr[i].value;
      context->psf[i] = state->fpr[i].paired1;
   }

   for (auto i = 0; i < 8; ++i) {
      context->gqr[i] = state->gqr[i].value;
   }

   context->cr = state->cr.value;
   context->lr = state->lr;
   context->ctr = state->ctr;
   context->xer = state->xer.value;
   //context->srr0 = state->sr[0];
   //context->srr1 = state->sr[1];
   context->fpscr = state->fpscr.value;
}

void
restoreContext(coreinit::OSContext *context)
{
   auto state = cpu::this_core::state();

   for (auto i = 0; i < 32; ++i) {
      state->gpr[i] = context->gpr[i];
   }

   for (auto i = 0; i < 32; ++i) {
      state->fpr[i].value = context->fpr[i];
      state->fpr[i].paired1 = context->psf[i];
   }

   for (auto i = 0; i < 8; ++i) {
      state->gqr[i].value = context->gqr[i];
   }

   state->cr.value = context->cr;
   state->lr = context->lr;
   state->ctr = context->ctr;
   state->xer.value = context->xer;
   //state->sr[0] = context->srr0;
   //state->sr[1] = context->srr1;
   state->fpscr.value = context->fpscr;

   state->reserve = false;
   state->reserveAddress = 0;
   state->reserveData = 0;
}

void
switchThread(coreinit::OSThread *previous, coreinit::OSThread *next)
{
   // Save our CIA for when we come back.
   auto core = cpu::this_core::state();

   uint32_t *stackTop = nullptr;
   if (previous) {
      core->gpr[1] -= 8;
      stackTop = mem::translate<uint32_t>(core->gpr[1]);
      stackTop[0] = core->cia;
      stackTop[1] = core->nia;
   }

   // If we have a current context, save it
   if (previous) {
      saveContext(&previous->context);
   }

   // We now effectively have nothing on the core
   core->nia = 0xFFFFFFFF;
   core->cia = 0xFFFFFFFF;

   // Switch to the new thread
   if (next) {
      if (!next->fiber) {
         next->fiber = allocateFiber(next);
      }

      restoreContext(&next->context);

      auto fiber = next->fiber;
      cpu::this_core::set_tracer(fiber->tracer);
      platform::swapToFiber(nullptr, fiber->handle);
   } else {
      // If we switch to the idle thread, set ourselves to
      // a known bad state to help with debugging...
      coreinit::OSContext blankContext = { 0 };
      restoreContext(&blankContext);

      cpu::this_core::set_tracer(nullptr);
      platform::swapToFiber(nullptr, tIdleFiber[core->id]);
   }

   checkDeadThread();

   if (stackTop) {
      core->cia = stackTop[0];
      core->nia = stackTop[1];
      core->gpr[1] += 8;
   }
}

}
