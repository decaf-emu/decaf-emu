#pragma once
#include "utils/structsize.h"
#include "utils/virtual_ptr.h"

namespace coreinit
{

/**
 * \defgroup coreinit_threadqueue Thread Queue
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSThread;

struct OSThreadLink
{
   be_ptr<OSThread> prev;
   be_ptr<OSThread> next;
};
CHECK_OFFSET(OSThreadLink, 0x00, prev);
CHECK_OFFSET(OSThreadLink, 0x04, next);
CHECK_SIZE(OSThreadLink, 0x8);

struct OSThreadQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
   be_ptr<void> parent;
   UNKNOWN(4);
};
CHECK_OFFSET(OSThreadQueue, 0x00, head);
CHECK_OFFSET(OSThreadQueue, 0x04, tail);
CHECK_OFFSET(OSThreadQueue, 0x08, parent);
CHECK_SIZE(OSThreadQueue, 0x10);

struct OSThreadSimpleQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
};
CHECK_OFFSET(OSThreadSimpleQueue, 0x00, head);
CHECK_OFFSET(OSThreadSimpleQueue, 0x04, tail);
CHECK_SIZE(OSThreadSimpleQueue, 0x08);

#pragma pack(pop)

void
OSInitThreadQueue(OSThreadQueue *queue);

void
OSInitThreadQueueEx(OSThreadQueue *queue, void *parent);

void
OSClearThreadQueue(OSThreadQueue *queue);

void
OSEraseFromThreadQueue(OSThreadQueue *queue, OSThread *thread);

void
OSInsertThreadQueue(OSThreadQueue *queue, OSThread *thread);

BOOL
OSIsThreadQueueEmpty(OSThreadQueue *queue);

OSThread *
OSPopFrontThreadQueue(OSThreadQueue *queue);

/** @} */

} // namespace coreinit
