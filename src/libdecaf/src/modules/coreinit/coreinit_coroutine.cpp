#include "coreinit.h"
#include "coreinit_coroutine.h"
#include "coreinit_thread.h"
#include "kernel/kernel_internal.h"

namespace coreinit
{

void
OSInitCoroutine(OSCoroutine *context, uint32_t entry, uint32_t stack)
{
   context->nia = entry;
   context->gpr1 = stack;
}

void
OSSwitchCoroutine(OSCoroutine *from, OSCoroutine *to)
{
   OSThread *thread = OSGetCurrentThread();

   // Save current coroutine state
   kernel::saveContext(&thread->context);
   from->nia = thread->context.lr;
   from->cr = thread->context.cr;
   from->gqr1 = thread->context.gqr[1];
   from->gpr1 = thread->context.gpr[1];
   from->gpr2 = thread->context.gpr[2];
   from->gpr13_31[0] = thread->context.gpr[13];
   for (int i = 14; i < 32; i++) {
      from->gpr13_31[i-13] = thread->context.gpr[i];
      from->fpr14_31[i-14] = thread->context.fpr[i];
      from->ps14_31[i-14][0] = static_cast<float>(thread->context.fpr[i]);
      from->ps14_31[i-14][1] = static_cast<float>(thread->context.psf[i]);
   }

   // Switch to new coroutine state
   thread->context.lr = to->nia;
   thread->context.cr = to->cr;
   thread->context.gqr[1] = to->gqr1;
   thread->context.gpr[0] = from->nia;  // Probably just coincidence
   thread->context.gpr[1] = to->gpr1;
   thread->context.gpr[2] = to->gpr2;
   thread->context.gpr[3] = 1;          // Possibly an argument?
   thread->context.gpr[4] = 1;          // Possibly an argument?
   thread->context.gpr[5] = to->gqr1;   // Probably just coincidence
   thread->context.gpr[13] = to->gpr13_31[0];
   for (int i = 14; i < 32; i++) {
      thread->context.gpr[i] = to->gpr13_31[i-13];
      thread->context.fpr[i] = to->fpr14_31[i-14];
      thread->context.psf[i] = static_cast<double>(to->ps14_31[i-14][1]);
   }
   kernel::restoreContext(&thread->context);
}

void
Module::registerCoroutineFunctions()
{
   RegisterKernelFunction(OSInitCoroutine);
   RegisterKernelFunction(OSSwitchCoroutine);
}

} // namespace coreinit
