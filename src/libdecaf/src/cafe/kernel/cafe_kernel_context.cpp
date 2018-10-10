#include "cafe_kernel_context.h"
#include "cafe_kernel_heap.h"

#include "cafe/cafe_ppc_interface_invoke.h"
#include "cafe/kernel/cafe_kernel.h"
#include "cafe/libraries/cafe_hle.h"

#include <array>
#include <common/platform_fiber.h>
#include <common/log.h>
#include <libcpu/cpu.h>

namespace cafe::kernel
{

struct HostContext
{
   platform::Fiber *fiber = nullptr;
   virt_ptr<Context> context = nullptr;
   cpu::Tracer *tracer = nullptr;
};

static std::array<virt_ptr<Context>, 3>
sCurrentContext;

static std::array<virt_ptr<Context>, 3>
sDeadContext;

static std::array<virt_ptr<Context>, 3>
sIdleContext;

constexpr auto CoreThreadStackSize = 0x100u;

struct StaticContextData
{
   be2_array<Context, 3> coreThreadContext;
   be2_array<std::byte, CoreThreadStackSize * 3> coreThreadStackBuffer;
};

static virt_ptr<StaticContextData>
sContextData;

static void
checkDeadContext();

using ContextEntryPoint = virt_func_ptr<void()>;

void
copyContextFromCpu(virt_ptr<Context> context)
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
   context->fpscr = state->fpscr.value;

   context->srr0 = state->srr0;
   context->dar = state->dar;
   context->dsisr = state->dsisr;
}

void
copyContextToCpu(virt_ptr<Context> context)
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
   state->fpscr.value = context->fpscr;

   state->srr0 = context->srr0;
   state->dar = context->dar;
   state->dsisr = context->dsisr;
}

void
sleepCurrentContext()
{
   // Grab the current core and context information
   auto core = cpu::this_core::state();
   auto context = sCurrentContext[core->id];
   decaf_check(context);

   // Save all our registers to the context
   copyContextFromCpu(context);
   context->nia = core->nia;
   context->cia = core->cia;

   // Some things to help us when debugging...
   core->nia = 0xFFFFFFFF;
   core->cia = 0xFFFFFFFF;
   cpu::this_core::setTracer(nullptr);
}

void
wakeCurrentContext()
{
   // Clean up any dead fibers
   checkDeadContext();

   // Grab the current core and context information
   auto core = cpu::this_core::state();
   auto context = sCurrentContext[core->id];
   decaf_check(context);

   // Restore our context from the OSContext
   copyContextToCpu(context);
   core->nia = context->nia;
   core->cia = context->cia;

   // Some things to help us when debugging...
   cpu::this_core::setTracer(context->hostContext->tracer);
}

static void
fiberEntryPoint(void *)
{
   // Load up the context
   wakeCurrentContext();

   // Invoke the PPC thread entry point, note we do not pass any arguments
   // because whoever created the thread would have already put the arguments
   // into the guest registers.
   auto core = cpu::this_core::state();
   auto exitPoint = virt_func_cast<ContextEntryPoint>(static_cast<virt_addr>(core->lr));
   auto entryPoint = virt_func_cast<ContextEntryPoint>(static_cast<virt_addr>(core->nia));

   invoke(core, entryPoint);
   invoke(core, exitPoint);
   decaf_check("Control flow returned to fiber entry point");
}

static void
freeHostContext(HostContext *hostContext)
{
   cpu::freeTracer(hostContext->tracer);
   platform::destroyFiber(hostContext->fiber);
   delete hostContext;
}

// This must be called under the same scheduler lock
// that added the thread to tDeadThread, we simply use
// the thread_local to pass it between fibers.
static void
checkDeadContext()
{
   auto coreId = cpu::this_core::id();
   auto deadContext = sDeadContext[coreId];

   if (deadContext) {
      sDeadContext[coreId] = nullptr;

      // Something broken if we are accidentally cleaning
      //  up currently active context...
      decaf_check(deadContext != sCurrentContext[coreId]);

      // Something is broken if we have no fiber
      decaf_check(deadContext->hostContext);

      // Destroy the fiber
      freeHostContext(deadContext->hostContext);
      deadContext->hostContext = nullptr;
   }
}

void
exitThreadNoLock()
{
   auto coreId = cpu::this_core::id();

   // Make sure exitThread is not called multiple times
   decaf_check(!sDeadContext[coreId]);

   // Mark this fiber to be cleaned up
   sDeadContext[coreId] = sCurrentContext[coreId];
}

static platform::Fiber *
getContextFiber(virt_ptr<Context> context)
{
   if (!context->hostContext) {
      context->hostContext = new HostContext();
      context->hostContext->tracer = cpu::allocTracer(1024 * 10 * 10);
      context->hostContext->fiber = platform::createFiber(fiberEntryPoint, nullptr);
      context->hostContext->context = context;
   }

   return context->hostContext->fiber;
}

void
resetFaultedContextFiber(virt_ptr<Context> context,
                         platform::FiberEntryPoint entry,
                         void *param)
{
   auto oldFiber = context->hostContext->fiber;
   setContextFiberEntry(context, entry, param);
   platform::swapToFiber(oldFiber, context->hostContext->fiber);
}

void
setContextFiberEntry(virt_ptr<Context> context,
                     platform::FiberEntryPoint entry,
                     void *param)
{
   if (!context->hostContext) {
      context->hostContext = new HostContext();
      context->hostContext->tracer = cpu::allocTracer(1024 * 10 * 10);
      context->hostContext->context = context;
   }

   context->hostContext->fiber = platform::createFiber(entry, param);
}

virt_ptr<Context>
getCurrentContext()
{
   return sCurrentContext[cpu::this_core::id()];
}

void
switchContext(virt_ptr<Context> next)
{
   // Don't do anything if we are switching to the same context.
   auto coreId = cpu::this_core::id();
   auto current = sCurrentContext[coreId];
   if (current == next) {
      return;
   }

   if (!next) {
      next = sIdleContext[coreId];
   }

   // Perform savage operations before the switch
   sleepCurrentContext();

   // Switch to the new fiber, note that coreId is no longer valid
   // after this point, as this context may have been switched to
   // a new core.
   sCurrentContext[coreId] = next;
   platform::swapToFiber(getContextFiber(current),
                         getContextFiber(next));

   // Perform restoral operations after the switch
   wakeCurrentContext();
}


/**
 * This should only be run from coreinit entry point, it will hijack the idle
 * context for core 1 and adopt it into coreinit's default thread 1.
 */
void
hijackCurrentHostContext(virt_ptr<Context> context)
{
   auto coreId = cpu::this_core::id();
   auto current = sCurrentContext[coreId];
   decaf_check(current == sIdleContext[1]);

   context->hostContext = current->hostContext;
   sCurrentContext[coreId] = context;

   // Reset the current core to an idle context
   current->hostContext = nullptr;
   setContextFiberEntry(
      current,
      [](void *core)
      {
         wakeCurrentContext();
         internal::idleCoreLoop(reinterpret_cast<cpu::Core *>(core));
      },
      cpu::this_core::state());
}

namespace internal
{

void
initialiseCoreContext(cpu::Core *core)
{
   // Allocate the root context
   auto context = virt_addrof(sContextData->coreThreadContext[core->id]);
   std::memset(context.get(), 0, sizeof(Context));

   auto stack = virt_addrof(sContextData->coreThreadStackBuffer[core->id * CoreThreadStackSize]);
   context->gpr[1] = virt_cast<virt_addr>(stack).getAddress() + CoreThreadStackSize - 8;
   context->attr |= 1 << core->id;

   // Setup host context for the root fiber
   context->hostContext = new HostContext();
   context->hostContext->tracer = cpu::allocTracer(1024 * 10 * 10);
   context->hostContext->fiber = platform::getThreadFiber();
   context->hostContext->context = context;

   // Save some needed information about the fiber run states.
   sIdleContext[core->id] = context;
   sCurrentContext[core->id] = context;
   sDeadContext[core->id] = nullptr;

   // Copy our core context to cpu core
   copyContextToCpu(context);

   // Set the core nia/cia to something debuggable
   core->nia = 0xFFFFFFF0u | core->id;
   core->cia = 0xFFFFFFF0u | core->id;
}

void
initialiseStaticContextData()
{
   sContextData = allocStaticData<StaticContextData>();
}

} // namespace internal

} // namespace cafe::kernel
