#include "coreinit.h"
#include "coreinit_coroutine.h"
#include "coreinit_thread.h"

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
   from->nia = thread->context.lr;
   from->cr = thread->context.cr;
   from->gpr5 = thread->context.gpr[5];
   from->gpr1 = thread->context.gpr[1];
   from->gpr2 = thread->context.gpr[2];
   for (int i = 13; i < 32; i++) {
      thread->context.gpr[i] = to->gpr13_31[i-13];
   }
   for (int i = 14; i < 32; i++) {
      thread->context.fpr[i] = to->fpr14_31[i-14];
   }

   // Switch to new coroutine state
   thread->context.lr = to->nia;
   thread->context.cr = to->cr;
   thread->context.gpr[0] = from->nia;  // Probably just coincidence
   thread->context.gpr[1] = to->gpr1;
   thread->context.gpr[2] = to->gpr2;
   thread->context.gpr[3] = 1;
   thread->context.gpr[4] = 1;
   thread->context.gpr[5] = to->gpr5;
   for (int i = 13; i < 32; i++) {
      thread->context.gpr[i] = to->gpr13_31[i-13];
   }
   for (int i = 14; i < 32; i++) {
      thread->context.fpr[i] = to->fpr14_31[i-14];
   }
}

void
Module::registerCoroutineFunctions()
{
   RegisterKernelFunction(OSInitCoroutine);
   RegisterKernelFunction(OSSwitchCoroutine);
}

} // namespace coreinit
