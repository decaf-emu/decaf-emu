#pragma once
#include "coreinit_time.h"
#include "systemobject.h"

enum class EventMode
{
   ManualReset = 0,
   AutoReset = 1
};

struct Fiber;

struct Event : public SystemObject
{
   static const uint32_t Tag = 0x65566E54;

   char *name;
   EventMode mode;
   std::atomic<bool> value;
   std::mutex mutex;
   std::vector<Fiber *> queue;
};

using EventHandle = SystemObjectHeader *;

void
OSInitEvent(EventHandle handle, bool value, EventMode mode);

void
OSInitEventEx(EventHandle handle, bool value, EventMode mode, char *name);

void
OSSignalEvent(EventHandle handle);

void
OSSignalEventAll(EventHandle handle);

void
OSWaitEvent(EventHandle handle);

void
OSResetEvent(EventHandle handle);

BOOL
OSWaitEventWithTimeout(EventHandle handle, Time timeout);
