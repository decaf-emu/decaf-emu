#include "ios_kernel_thread.h"
#include "ios_kernel_threadqueue.h"

namespace ios::kernel
{

void
ThreadQueue_Initialise(phys_ptr<ThreadQueue> queue)
{
   queue->first = nullptr;
}

void
ThreadQueue_PushThread(phys_ptr<ThreadQueue> queue,
                       phys_ptr<Thread> thread)
{
   if (!queue || !thread) {
      return;
   }

   auto insertAt = phys_addrof(queue->first);
   auto next = queue->first;

   for (auto itr = queue->first; itr; itr = itr->threadQueueNext) {
      if (thread->priority < itr->priority) {
         break;
      }

      insertAt = phys_addrof(itr->threadQueueNext);
      next = itr->threadQueueNext;
   }

   *insertAt = thread;
   thread->threadQueue = queue;
   thread->threadQueueNext = next;
}

phys_ptr<Thread>
ThreadQueue_PeekThread(phys_ptr<ThreadQueue> queue)
{
   return queue->first;
}

phys_ptr<Thread>
ThreadQueue_PopThread(phys_ptr<ThreadQueue> queue)
{
   auto thread = queue->first;

   if (thread) {
      queue->first = thread->threadQueueNext;
      thread->threadQueue = nullptr;
      thread->threadQueueNext = nullptr;
   } else {
      queue->first = nullptr;
   }

   return thread;
}

void
ThreadQueue_RemoveThread(phys_ptr<ThreadQueue> queue,
                         phys_ptr<Thread> removeThread)
{
   if (!queue || !removeThread) {
      return;
   }

   auto thread = queue->first;
   auto removeAt = phys_addrof(queue->first);

   while (thread) {
      if (thread == removeThread) {
         *removeAt = removeThread->threadQueueNext;
         break;
      }

      removeAt = phys_addrof(thread->threadQueueNext);
      thread = thread->threadQueueNext;
   }
}

} // namespace ios::kernel
