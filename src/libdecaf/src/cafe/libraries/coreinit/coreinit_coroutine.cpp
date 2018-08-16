#include "coreinit.h"
#include "coreinit_coroutine.h"
#include "coreinit_thread.h"
#include "cafe/kernel/cafe_kernel_context.h"

namespace cafe::coreinit
{

void
OSInitCoroutine(virt_ptr<OSCoroutine> context,
                uint32_t entry,
                uint32_t stack)
{
   context->lr = entry;
   context->gpr1 = stack;
}

uint32_t
OSLoadCoroutine(virt_ptr<OSCoroutine> coroutine,
                uint32_t returnValue)
{
   auto thread = OSGetCurrentThread();
   auto context = virt_addrof(thread->context);

   // Switch to new coroutine state
   context->lr = coroutine->lr;
   context->cr = coroutine->cr;
   context->gqr[1] = coroutine->gqr1;
   context->gpr[1] = coroutine->gpr1;
   context->gpr[2] = coroutine->gpr2;
   context->gpr[3] = returnValue;
   context->gpr[13] = coroutine->gpr13_31[0];
   for (auto i = 14; i < 32; i++) {
      context->gpr[i] = coroutine->gpr13_31[i - 13];
      context->fpr[i] = coroutine->fpr14_31[i - 14];
      context->psf[i] = static_cast<double>(coroutine->ps14_31[i - 14][1]);
   }

   // Copy context to CPU registers
   cafe::kernel::copyContextToCpu(context);
   return returnValue;
}

uint32_t
OSSaveCoroutine(virt_ptr<OSCoroutine> coroutine)
{
   // Copy CPU registers to context
   auto thread = OSGetCurrentThread();
   auto context = virt_addrof(thread->context);
   cafe::kernel::copyContextFromCpu(context);

   // Update the coroutine with context registers
   coroutine->lr = context->lr;
   coroutine->cr = context->cr;
   coroutine->gqr1 = context->gqr[1];
   coroutine->gpr1 = context->gpr[1];
   coroutine->gpr2 = context->gpr[2];
   coroutine->gpr13_31[0] = context->gpr[13];
   for (auto i = 14; i < 32; i++) {
      coroutine->gpr13_31[i - 13] = context->gpr[i];
      coroutine->fpr14_31[i - 14] = context->fpr[i];
      coroutine->ps14_31[i - 14][0] = static_cast<float>(context->fpr[i]);
      coroutine->ps14_31[i - 14][1] = static_cast<float>(context->psf[i]);
   }

   return 0;
}

void
OSSwitchCoroutine(virt_ptr<OSCoroutine> from,
                  virt_ptr<OSCoroutine> to)
{
   if (OSSaveCoroutine(from) == 0) {
      OSLoadCoroutine(to, 1);
   }
}

void
Library::registerCoroutineSymbols()
{
   RegisterFunctionExport(OSInitCoroutine);
   RegisterFunctionExport(OSLoadCoroutine);
   RegisterFunctionExport(OSSaveCoroutine);
   RegisterFunctionExport(OSSwitchCoroutine);
}

} // namespace cafe::coreinit
