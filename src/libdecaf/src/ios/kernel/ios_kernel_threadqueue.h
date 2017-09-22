#pragma once
#include <common/structsize.h>
#include <libcpu/be2_struct.h>

namespace ios::kernel
{

struct Thread;

struct ThreadQueue
{
   //! Linked list on thread->threadQueueNext.
   be2_phys_ptr<Thread> first;
};
CHECK_OFFSET(ThreadQueue, 0x0, first);
CHECK_SIZE(ThreadQueue, 0x4);

void
ThreadQueue_Initialise(phys_ptr<ThreadQueue> queue);

void
ThreadQueue_PushThread(phys_ptr<ThreadQueue> queue,
                       phys_ptr<Thread> thread);

phys_ptr<Thread>
ThreadQueue_PeekThread(phys_ptr<ThreadQueue> queue);

phys_ptr<Thread>
ThreadQueue_PopThread(phys_ptr<ThreadQueue> queue);

void
ThreadQueue_RemoveThread(phys_ptr<ThreadQueue> queue,
                         phys_ptr<Thread> removeThread);

} // namespace ios::kernel
