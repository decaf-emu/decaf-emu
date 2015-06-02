#pragma once
#include "systemtypes.h"

#pragma pack(push, 1)

struct OSThread;

struct OSThreadQueue
{
   be_ptr<OSThread> head;
   be_ptr<OSThread> tail;
   be_ptr<void> parent;
   be_val<uint32_t> unk1;
};

#pragma pack(pop)

p32<OSThread>
OSGetCurrentThread();

void
OSInitThreadQueue(p32<OSThreadQueue> pQueue);

void
OSInitThreadQueueEx(p32<OSThreadQueue> pQueue, p32<void> pParent);
