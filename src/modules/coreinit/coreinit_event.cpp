#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_event.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "system.h"

namespace coreinit
{

const uint32_t OSEvent::Tag;


/**
 * Initialises an event structure.
 */
void
OSInitEvent(OSEvent *event, bool value, OSEventMode mode)
{
   OSInitEventEx(event, value, mode, nullptr);
}


/**
* Initialises an event structure.
*/
void
OSInitEventEx(OSEvent *event, bool value, OSEventMode mode, char *name)
{
   event->tag = OSEvent::Tag;
   event->mode = mode;
   event->value = value;
   event->name = name;
   OSInitThreadQueueEx(&event->queue, event);
}


/**
* Signal an event.
*
* This will set the events signal value to true.
*
* In auto reset mode if at least one thread is in the queue it will:
* - Reset the value back to FALSE
* - Wake up the first thread in the waiting queue
*
* In manual reset mode:
* - Wake up all threads in the waiting queue
* - The event value remains TRUE until the user calls OSResetEvent
*/
void
OSSignalEvent(OSEvent *event)
{
   coreinit::internal::lockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value != FALSE) {
      // Event has already been set
      coreinit::internal::unlockScheduler();
      return;
   }

   // Set event
   event->value = TRUE;

   if (!OSIsThreadQueueEmpty(&event->queue)) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset value
         event->value = FALSE;

         // Wakeup one thread
         auto thread = OSPopFrontThreadQueue(&event->queue);
         coreinit::internal::wakeupOneThreadNoLock(thread);
         coreinit::internal::rescheduleNoLock();
      } else {
         // Wakeup all threads
         coreinit::internal::wakeupThreadNoLock(&event->queue);
         coreinit::internal::rescheduleNoLock();
      }
   }

   coreinit::internal::unlockScheduler();
}


/**
 * Signal the event and wakeup all waiting threads regardless of mode
 *
 * This differs to OSSignalEvent only for auto reset mode where it
 * will wake up all threads instead of just one.
 *
 * If there is at least one thread woken up and the alarm is in
 * auto reset mode then the event value is reset to FALSE.
 */
void
OSSignalEventAll(OSEvent *event)
{
   coreinit::internal::lockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value != FALSE) {
      // Event has already been set
      coreinit::internal::unlockScheduler();
      return;
   }

   // Set event
   event->value = TRUE;

   if (!OSIsThreadQueueEmpty(&event->queue)) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      // Wakeup all threads
      coreinit::internal::wakeupThreadNoLock(&event->queue);
      coreinit::internal::rescheduleNoLock();
   }

   coreinit::internal::unlockScheduler();
}


/**
 * Reset the event value to FALSE
 */
void
OSResetEvent(OSEvent *event)
{
   coreinit::internal::lockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   // Reset event
   event->value = FALSE;

   coreinit::internal::unlockScheduler();
}


/**
 * Wait for the event value to become TRUE.
 *
 * If the event value is already TRUE then this will return immediately,
 * and will set the event value to FALSE if the event is in auto reset mode.
 *
 * If the event value is FALSE then the thread will sleep until the event
 * is signalled by another thread.
 */
void
OSWaitEvent(OSEvent *event)
{
   coreinit::internal::lockScheduler();
   assert(event);
   assert(event->tag == OSEvent::Tag);

   if (event->value) {
      // Event is already set

      if (event->mode == OSEventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      coreinit::internal::unlockScheduler();
      return;
   } else {
      // Wait for event to be set
      coreinit::internal::sleepThreadNoLock(&event->queue);
      coreinit::internal::rescheduleNoLock();
   }

   coreinit::internal::unlockScheduler();
}

static AlarmCallback
pEventAlarmHandler = nullptr;

struct EventAlarmData
{
   OSEvent *event;
   OSThread *thread;
   BOOL timeout;
};

static void
EventAlarmHandler(OSAlarm *alarm, OSContext *context)
{
   // Wakeup the thread waiting on this alarm
   auto data = reinterpret_cast<EventAlarmData*>(OSGetAlarmUserData(alarm));
   data->timeout = TRUE;
   coreinit::internal::wakeupOneThreadNoLock(data->thread);
}


/**
 * Wait for an event value to be TRUE with a timeout
 *
 * Behaves the same than OSWaitEvent but with a timeout.
 *
 * Returns TRUE if the event was signalled, FALSE if wait timed out.
 */
BOOL
OSWaitEventWithTimeout(OSEvent *event, OSTime timeout)
{
   BOOL result = TRUE;
   coreinit::internal::lockScheduler();

   // Check if event is already set
   if (event->value) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      coreinit::internal::unlockScheduler();
      return TRUE;
   }

   // Setup some alarm data for callback
   auto data = coreinit::internal::sysAlloc<EventAlarmData>();
   data->event = event;
   data->thread = OSGetCurrentThread();
   data->timeout = FALSE;

   // Create an alarm to trigger timeout
   auto alarm = coreinit::internal::sysAlloc<OSAlarm>();
   OSCreateAlarm(alarm);
   OSSetAlarmUserData(alarm, data);
   OSSetAlarm(alarm, timeout, pEventAlarmHandler);

   // Wait for the event
   coreinit::internal::sleepThreadNoLock(&event->queue);
   coreinit::internal::rescheduleNoLock();

   if (data->timeout) {
      // Timed out, remove from wait queue
      OSEraseFromThreadQueue(&event->queue, data->thread);
      coreinit::internal::unlockScheduler();
      result = FALSE;
   } else {
      // Did not time out, cancel pending alarm
      coreinit::internal::unlockScheduler();
      OSCancelAlarm(alarm);
   }

   coreinit::internal::sysFree(data);
   coreinit::internal::sysFree(alarm);
   return result;
}

void
Module::registerEventFunctions()
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
Module::initialiseEvent()
{
   pEventAlarmHandler = findExportAddress("EventAlarmHandler");
}

} // namespace coreinit
