#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_event.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "system.h"

const uint32_t OSEvent::Tag;

void
OSInitEvent(OSEvent *event, bool value, EventMode mode)
{
   OSInitEventEx(event, value, mode, nullptr);
}

void
OSInitEventEx(OSEvent *event, bool value, EventMode mode, char *name)
{
   event->tag = OSEvent::Tag;
   event->mode = mode;
   event->value = value;
   event->name = name;
   OSInitThreadQueueEx(&event->queue, event);
}

void
OSSignalEvent(OSEvent *event)
{
   OSLockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value != FALSE) {
      // Event has already been set
      OSUnlockScheduler();
      return;
   }

   // Set event
   event->value = TRUE;

   if (!OSIsThreadQueueEmpty(&event->queue)) {
      if (event->mode == EventMode::AutoReset) {
         // Reset value
         event->value = FALSE;

         // Wakeup one thread
         auto thread = OSPopFrontThreadQueue(&event->queue);
         OSWakeupOneThreadNoLock(thread);
         OSRescheduleNoLock();
      } else {
         // Wakeup all threads
         OSWakeupThreadNoLock(&event->queue);
         OSRescheduleNoLock();
      }
   }

   OSUnlockScheduler();
}

void
OSSignalEventAll(OSEvent *event)
{
   OSLockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value != FALSE) {
      // Event has already been set
      OSUnlockScheduler();
      return;
   }

   // Set event
   event->value = TRUE;

   if (!OSIsThreadQueueEmpty(&event->queue)) {
      if (event->mode == EventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      // Wakeup all threads
      OSWakeupThreadNoLock(&event->queue);
      OSRescheduleNoLock();
   }

   OSUnlockScheduler();
}

void
OSResetEvent(OSEvent *event)
{
   OSLockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   // Reset event
   event->value = FALSE;

   OSUnlockScheduler();
}

void
OSWaitEvent(OSEvent *event)
{
   OSLockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value) {
      // Event is already set

      if (event->mode == EventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      OSUnlockScheduler();
      return;
   } else {
      // Wait for event to be set
      OSSleepThreadNoLock(&event->queue);
      OSRescheduleNoLock();
   }

   OSUnlockScheduler();
}

static AlarmCallback
pEventAlarmHandler = nullptr;

struct EventAlarmData
{
   OSEvent *event;
   OSThread *thread;
   BOOL timeout;
};

void
EventAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   OSLockScheduler();

   // Wakeup the thread waiting on this alarm
   auto data = reinterpret_cast<EventAlarmData*>(OSGetAlarmUserData(alarm));
   data->timeout = TRUE;
   OSWakeupOneThreadNoLock(data->thread);

   OSUnlockScheduler();
}

BOOL
OSWaitEventWithTimeout(OSEvent *event, OSTime timeout)
{
   BOOL result = TRUE;
   OSLockScheduler();

   // Check if event is already set
   if (event->value) {
      if (event->mode == EventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      OSUnlockScheduler();
      return TRUE;
   }

   // Setup some alarm data for callback
   auto data = OSAllocFromSystem<EventAlarmData>();
   data->event = event;
   data->thread = OSGetCurrentThread();
   data->timeout = FALSE;

   // Unlock scheduler to setup alarm
   OSUnlockScheduler();

   // Create an alarm to trigger timeout
   auto alarm = OSAllocFromSystem<OSAlarm>();
   OSCreateAlarm(alarm);
   OSSetAlarmUserData(alarm, data);
   OSSetAlarm(alarm, timeout, pEventAlarmHandler);

   // Wait for the event
   OSLockScheduler();
   OSSleepThreadNoLock(&event->queue);
   OSRescheduleNoLock();

   if (data->timeout) {
      // Timed out, remove from wait queue
      OSEraseFromThreadQueue(&event->queue, data->thread);
      result = FALSE;
   }

   OSFreeToSystem(data);
   OSFreeToSystem(alarm);
   OSUnlockScheduler();
   return result;
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
   RegisterKernelFunction(EventAlarmHandler);
}

void
CoreInit::initialiseEvent()
{
   pEventAlarmHandler = findExportAddress("EventAlarmHandler");
}
