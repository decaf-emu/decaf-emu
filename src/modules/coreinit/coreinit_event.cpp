#include "coreinit.h"
#include "coreinit_event.h"
#include "system.h"

void
OSInitEvent(EventHandle handle, BOOL value, EventMode mode)
{
   OSInitEventEx(handle, value, mode, nullptr);
}

void
OSInitEventEx(EventHandle handle, BOOL value, EventMode mode, char *name)
{
   auto event = gSystem.addSystemObject<Event>(handle);
   event->mode = mode;
   event->value = value;
   event->name = name;
}

void
OSSignalEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   event->value = TRUE;

   if (event->mode == EventMode::ManualReset) {
      event->condition.notify_all();
   } else if (event->mode == EventMode::AutoReset) {
      event->condition.notify_one();
   }
}

void
OSSignalEventAll(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   event->value = TRUE;
   event->condition.notify_all();
}

void
OSResetEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   event->value = FALSE;
}

void
OSWaitEvent(EventHandle handle)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   auto lock = std::unique_lock<std::mutex> { event->mutex };

   if (!event->value == TRUE) {
      event->condition.wait(lock, [event] { return event->value; });
   }

   if (event->mode == EventMode::AutoReset) {
      event->value = FALSE;
   }
}

BOOL
OSWaitEventWithTimeout(EventHandle handle, Time timeout)
{
   auto event = gSystem.getSystemObject<Event>(handle);
   auto lock = std::unique_lock<std::mutex> { event->mutex };

   if (!event->value == TRUE) {
      if (!event->condition.wait_for(lock, std::chrono::nanoseconds(timeout), [event] { return event->value; })) {
         return FALSE;
      }
   }

   if (event->mode == EventMode::AutoReset) {
      event->value = FALSE;
   }

   return TRUE;
}

void
CoreInit::registerEventFunctions()
{
   RegisterKernelFunction(OSInitEvent);
   RegisterKernelFunction(OSInitEventEx);
   RegisterKernelFunction(OSSignalEvent);
   RegisterKernelFunction(OSSignalEventAll);
   RegisterKernelFunction(OSResetEvent);
   RegisterKernelFunction(OSWaitEvent);
   RegisterKernelFunction(OSWaitEventWithTimeout);
}
