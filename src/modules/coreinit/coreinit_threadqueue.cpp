#include "coreinit.h"
#include "coreinit_thread.h"
#include "coreinit_threadqueue.h"
#include "coreinit_internal_queue.h"

namespace coreinit
{

namespace internal
{

bool ThreadQueueSortFunc(OSThread *a, OSThread *b) {
   return a->priority < b->priority;
}

using ThreadQueueFuncs = SortedQueueFuncs < OSThreadQueue, OSThreadLink, OSThread, &OSThread::link, ThreadQueueSortFunc> ;

}

void
OSInitThreadQueue(OSThreadQueue *queue)
{
   internal::ThreadQueueFuncs::init(queue);
   queue->parent = nullptr;
}

void
OSInitThreadQueueEx(OSThreadQueue *queue, void *parent)
{
   internal::ThreadQueueFuncs::init(queue);
   queue->parent = parent;
}

void
OSClearThreadQueue(OSThreadQueue *queue)
{
   internal::ThreadQueueFuncs::clear(queue);
}

void
OSEraseFromThreadQueue(OSThreadQueue *queue, OSThread *thread)
{
   internal::ThreadQueueFuncs::erase(queue, thread);
}

void
OSInsertThreadQueue(OSThreadQueue *queue, OSThread *thread)
{
   internal::ThreadQueueFuncs::insert(queue, thread);
   thread->queue = queue;
}

BOOL
OSIsThreadQueueEmpty(OSThreadQueue *queue)
{
   return internal::ThreadQueueFuncs::isEmpty(queue);
}

OSThread *
OSPopFrontThreadQueue(OSThreadQueue *queue)
{
   return internal::ThreadQueueFuncs::popFront(queue);
}

} // namespace coreinit
