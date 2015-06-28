#pragma once
#include <condition_variable>
#include <mutex>
#include "coreinit_time.h"
#include "systemobject.h"

enum class EventMode
{
   ManualReset = 0,
   AutoReset = 1
};

struct Event : public SystemObject
{
   static const uint32_t Tag = 0x65566E54;

   char *name;
   EventMode mode;
   BOOL value;
   std::mutex mutex;
   std::condition_variable condition;
};

using EventHandle = p32<SystemObjectHeader>;

void
OSInitEvent(EventHandle handle, BOOL value, EventMode mode);

void
OSInitEventEx(EventHandle handle, BOOL value, EventMode mode, char *name);

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
