#include "coreinit.h"
#include "coreinit_thread.h"
#include "coreinit_threadqueue.h"
#include "coreinit_queue.h"

namespace coreinit
{

void
OSInitThreadQueue(OSThreadQueue *queue)
{
   OSInitQueueEx(queue, nullptr);
}

void
OSInitThreadQueueEx(OSThreadQueue *queue, void *parent)
{
   OSInitQueueEx(queue, parent);
}

void
OSClearThreadQueue(OSThreadQueue *queue)
{
   OSClearQueue(queue);
}

void
OSEraseFromThreadQueue(OSThreadQueue *queue, OSThread *thread)
{
   OSEraseFromQueue(queue, thread);
}

void
OSInsertThreadQueue(OSThreadQueue *queue, OSThread *thread)
{
   thread->queue = queue;

   if (!queue->head) {
      // Insert only item
      thread->link.prev = nullptr;
      thread->link.next = nullptr;
      queue->head = thread;
      queue->tail = thread;
   } else {
      OSThread *insertBefore = nullptr;

      // Find insert location based on priority
      for (insertBefore = queue->head; insertBefore; insertBefore = insertBefore->link.next) {
         if (insertBefore->basePriority > thread->priority) {
            break;
         }
      }

      if (!insertBefore) {
         // Insert at tail
         queue->tail->link.next = thread;
         thread->link.next = nullptr;
         thread->link.prev = queue->tail;
         queue->tail = thread;
      } else {
         // Insert in middle
         thread->link.next = insertBefore;
         thread->link.prev = insertBefore->link.prev;

         if (insertBefore->link.prev) {
            insertBefore->link.prev->link.next = thread;
         }

         insertBefore->link.prev = thread;
      }
   }
}

BOOL
OSIsThreadQueueEmpty(OSThreadQueue *queue)
{
   return OSIsEmptyQueue(queue);
}

OSThread *
OSPopFrontThreadQueue(OSThreadQueue *queue)
{
   return OSPopFrontFromQueue(queue);
}

} // namespace coreinit
