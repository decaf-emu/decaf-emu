#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_event.h"
#include "coreinit_memheap.h"
#include "coreinit_scheduler.h"
#include "coreinit_time.h"
#include "ppcutils/stackobject.h"
#include <common/decaf_assert.h>

namespace coreinit
{

const uint32_t
OSEvent::Tag;


/**
 * Initialises an event structure.
 */
void
OSInitEvent(OSEvent *event,
            BOOL value,
            OSEventMode mode)
{
   OSInitEventEx(event, value, mode, nullptr);
}


/**
 * Initialises an event structure.
 */
void
OSInitEventEx(OSEvent *event,
              BOOL value,
              OSEventMode mode,
              char *name)
{
   decaf_check(event);
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
 * In auto reset mode:
 * - Will wake up the first thread possible to wake
 * - If a thread is woken, the event value is reset to FALSE
 *
 * In manual reset mode:
 * - Wakes up all possible threads in the waiting queue
 * - The event value remains TRUE until the user calls OSResetEvent
 */
void
OSSignalEvent(OSEvent *event)
{
   internal::lockScheduler();
   decaf_check(event);
   decaf_check(event->tag == OSEvent::Tag);

   if (event->value) {
      // Event has already been set
      internal::unlockScheduler();
      return;
   }

   // Set the event
   event->value = TRUE;

   if (event->mode == OSEventMode::AutoReset) {
      if (!internal::ThreadQueue::empty(&event->queue)) {
         OSThread *wakeThread = nullptr;

         // Find the first thread that we can wake
         for (auto thread = event->queue.head; thread; thread = thread->link.next) {
            decaf_check(thread->queue == &event->queue);

            if (thread->waitEventTimeoutAlarm) {
               if (!internal::cancelAlarm(thread->waitEventTimeoutAlarm)) {
                  // If we could not cancel the alarm, do not wake this thread
                  continue;
               }
            }

            wakeThread = thread;
            break;
         }

         if (wakeThread) {
            // Reset the event
            event->value = FALSE;
            internal::wakeupOneThreadNoLock(wakeThread);
         }

         internal::rescheduleAllCoreNoLock();
      }
   } else {
      // Wake all possible threads
      for (auto thread = event->queue.head; thread; ) {
         decaf_check(thread->queue == &event->queue);

         // Save thread->link.next as it will be reset by wakeupOneThreadNoLock
         auto next = thread->link.next;

         if (thread->waitEventTimeoutAlarm) {
            if (!internal::cancelAlarm(thread->waitEventTimeoutAlarm)) {
               // If we could not cancel the alarm, do not wake this thread
               thread = next;
               continue;
            }
         }

         internal::wakeupOneThreadNoLock(thread);
         thread = next;
      }

      internal::rescheduleAllCoreNoLock();
   }

   internal::unlockScheduler();
}


/**
 * Signal the event and wakeup all waiting threads.
 *
 * In manual reset mode:
 * - Wakes up all possible threads in the waiting queue
 * - The event value will always be set to TRUE.
 *
 * In auto reset mode:
 * - Wakes up all possible threads in the waiting queue
 * - The event value will only be set to TRUE if no threads are woken.
 */
void
OSSignalEventAll(OSEvent *event)
{
   internal::lockScheduler();
   decaf_check(event);
   decaf_check(event->tag == OSEvent::Tag);

   if (event->value) {
      // Event has already been set
      internal::unlockScheduler();
      return;
   }

   // Manual reset always sets the event value to TRUE
   if (event->mode == OSEventMode::ManualReset) {
      event->value = TRUE;
   }

   if (!internal::ThreadQueue::empty(&event->queue)) {
      auto threadsWoken = 0u;

      // Wake any waiting threads
      for (auto thread = event->queue.head; thread; thread = thread->link.next) {
         decaf_check(thread->queue == &event->queue);

         if (thread->waitEventTimeoutAlarm) {
            if (!internal::cancelAlarm(thread->waitEventTimeoutAlarm)) {
               // If we could not cancel the alarm, do not wake this thread
               continue;
            }
         }

         internal::wakeupOneThreadNoLock(thread);
         threadsWoken++;
      }

      // Auto reset will only set the event value if no threads were woken
      if (event->mode == OSEventMode::AutoReset && threadsWoken == 0) {
         event->value = TRUE;
      }

      internal::rescheduleAllCoreNoLock();
   }

   internal::unlockScheduler();
}


/**
 * Reset the event value to FALSE
 */
void
OSResetEvent(OSEvent *event)
{
   internal::lockScheduler();
   decaf_check(event);
   decaf_check(event->tag == OSEvent::Tag);

   // Reset event
   event->value = FALSE;

   internal::unlockScheduler();
}


/**
 * Wait for the event value to become TRUE.
 *
 * If the event value is already TRUE:
 * - Returns immediately, no wait is performed.
 * - Sets event value to FALSE if in AutoReset mode.
 *
 * If the event value is FALSE:
 * - The current thread will go to sleep until the event is signalled by another thread.
 */
void
OSWaitEvent(OSEvent *event)
{
   internal::lockScheduler();
   decaf_check(event);

   // HACK: Naughty Bayonetta not initialising event before using it.
   // decaf_check(event->tag == OSEvent::Tag);
   if (event->tag != OSEvent::Tag) {
      OSInitEvent(event, false, OSEventMode::ManualReset);
   }

   // Check if the event is already set
   if (event->value) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }
   } else {
      // Wait for event to be set
      internal::sleepThreadNoLock(&event->queue);
      internal::rescheduleSelfNoLock();
   }

   internal::unlockScheduler();
}


static AlarmCallback
sEventAlarmHandler = nullptr;

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

   // Remove this alarm from the thread
   data->thread->waitEventTimeoutAlarm = nullptr;

   // System Alarm, we already have the scheduler lock
   internal::wakeupOneThreadNoLock(data->thread);
}


/**
 * Wait for an event value to be TRUE with a timeout
 *
 * Behaves the same than OSWaitEvent but with a timeout.
 *
 * Returns TRUE if the event was signalled, FALSE if wait timed out.
 */
BOOL
OSWaitEventWithTimeout(OSEvent *event,
                       OSTime timeoutNS)
{
   ppcutils::StackObject<EventAlarmData> data;
   ppcutils::StackObject<OSAlarm> alarm;

   internal::lockScheduler();

   // Check if event is already set
   if (event->value) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset event
         event->value = FALSE;
      }

      internal::unlockScheduler();
      return TRUE;
   }

   // Setup some alarm data for callback
   auto thread = OSGetCurrentThread();
   data->event = event;
   data->thread = thread;
   data->timeout = FALSE;

   // Create an alarm to trigger timeout
   auto timeoutTicks = internal::nsToTicks(timeoutNS);
   OSCreateAlarm(alarm);
   internal::setAlarmInternal(alarm, timeoutTicks, sEventAlarmHandler, data);

   // Set waitEventTimeoutAlarm so we can cancel it when event is signalled
   thread->waitEventTimeoutAlarm = alarm;

   // Wait for the event
   internal::sleepThreadNoLock(&event->queue);
   internal::rescheduleAllCoreNoLock();

   // Clear waitEventTimeoutAlarm
   thread->waitEventTimeoutAlarm = nullptr;

   auto result = FALSE;

   if (event->value) {
      if (event->mode == OSEventMode::AutoReset) {
         // Reset the event if its in auto-reset mode
         event->value = FALSE;
      }

      result = TRUE;
   } else if (!data->timeout) {
      result = TRUE;
   }

   internal::unlockScheduler();
   return result;
}

void
Module::initialiseEvent()
{
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

   RegisterInternalFunction(EventAlarmHandler, sEventAlarmHandler);
}

} // namespace coreinit
