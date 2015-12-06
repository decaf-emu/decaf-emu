#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_spinlock.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "coreinit_queue.h"
#include "utils/wfunc_call.h"
#include "processor.h"

static OSSpinLock *
gAlarmLock;

static OSAlarmQueue *
gAlarmQueue[CoreCount];

const uint32_t
OSAlarm::Tag;

const uint32_t
OSAlarmQueue::Tag;


/**
 * Internal alarm cancel.
 *
 * Reset the alarm state to cancelled.
 * Wakes up all threads waiting on the alarm.
 * Removes the alarm from any queue it is in.
 */
static BOOL
cancelAlarmNoLock(OSAlarm *alarm)
{
   if (alarm->state != OSAlarmState::Set) {
      return FALSE;
   }

   alarm->state = OSAlarmState::Cancelled;
   alarm->nextFire = 0;
   alarm->period = 0;

   if (alarm->alarmQueue) {
      OSEraseFromQueue<OSAlarmQueue>(alarm->alarmQueue, alarm);
   }

   OSWakeupThread(&alarm->threadQueue);
   return TRUE;
}


/**
 * Cancel an alarm.
 */
BOOL
OSCancelAlarm(OSAlarm *alarm)
{
   ScopedSpinLock lock(gAlarmLock);
   return cancelAlarmNoLock(alarm);
}


/**
 * Cancel all alarms which have a matching tag.
 */
void
OSCancelAlarms(uint32_t group)
{
   ScopedSpinLock lock(gAlarmLock);

   for (auto i = 0u; i < 3; ++i) {
      auto queue = gAlarmQueue[i];

      for (OSAlarm *alarm = queue->head; alarm; ) {
         auto next = alarm->link.next;

         if (alarm->group == group) {
            cancelAlarmNoLock(alarm);
         }

         alarm = next;
      }
   }
}


/**
 * Initialise an alarm structure.
 */
void
OSCreateAlarm(OSAlarm *alarm)
{
   OSCreateAlarmEx(alarm, nullptr);
}


/**
* Initialise an alarm structure.
*/
void
OSCreateAlarmEx(OSAlarm *alarm, const char *name)
{
   memset(alarm, 0, sizeof(OSAlarm));
   alarm->tag = OSAlarm::Tag;
   alarm->name = name;
   OSInitThreadQueueEx(&alarm->threadQueue, alarm);
}


/**
* Return the user data stored in the alarm using OSSetAlarmUserData
*/
void *
OSGetAlarmUserData(OSAlarm *alarm)
{
   return alarm->userData;
}


/**
 * Initialise an alarm queue structure
 */
void
OSInitAlarmQueue(OSAlarmQueue *queue)
{
   memset(queue, 0, sizeof(OSAlarmQueue));
   queue->tag = OSAlarmQueue::Tag;
}


/**
 * Set a one shot alarm to perform a callback after an amount of time.
 */
BOOL
OSSetAlarm(OSAlarm *alarm, OSTime time, AlarmCallback callback)
{
   return OSSetPeriodicAlarm(alarm, OSGetTime() + time, 0, callback);
}


/**
 * Set a repeated alarm to execute a callback every interval from start.
 */
BOOL
OSSetPeriodicAlarm(OSAlarm *alarm, OSTime start, OSTime interval, AlarmCallback callback)
{
   ScopedSpinLock lock(gAlarmLock);

   // Set alarm
   alarm->nextFire = start;
   alarm->callback = callback;
   alarm->period = interval;
   alarm->context = nullptr;
   alarm->state = OSAlarmState::Set;

   // Erase from old alarm queue
   if (alarm->alarmQueue) {
      OSEraseFromQueue(static_cast<OSAlarmQueue*>(alarm->alarmQueue), alarm);
   }

   // Add to this core's alarm queue
   auto core = OSGetCoreId();
   auto queue = gAlarmQueue[core];
   alarm->alarmQueue = queue;
   OSAppendQueue(queue, alarm);

   // Set the interrupt timer in processor
   gProcessor.setInterruptTimer(core, OSTimeToChrono(alarm->nextFire));
   return TRUE;
}


/**
 * Set an alarm tag which is used in OSCancelAlarms for bulk cancellation.
 */
void
OSSetAlarmTag(OSAlarm *alarm, uint32_t group)
{
   OSUninterruptibleSpinLock_Acquire(gAlarmLock);
   alarm->group = group;
   OSUninterruptibleSpinLock_Release(gAlarmLock);
}


/**
 * Set alarm user data which is returned by OSGetAlarmUserData.
 */
void
OSSetAlarmUserData(OSAlarm *alarm, void *data)
{
   OSUninterruptibleSpinLock_Acquire(gAlarmLock);
   alarm->userData = data;
   OSUninterruptibleSpinLock_Release(gAlarmLock);
}


/**
 * Sleep the current thread until the alarm has been triggered or cancelled.
 */
BOOL
OSWaitAlarm(OSAlarm *alarm)
{
   OSLockScheduler();
   OSUninterruptibleSpinLock_Acquire(gAlarmLock);
   BOOL result = FALSE;
   assert(alarm);
   assert(alarm->tag == OSAlarm::Tag);

   if (alarm->state != OSAlarmState::Set) {
      OSUninterruptibleSpinLock_Release(gAlarmLock);
      OSUnlockScheduler();
      return FALSE;
   }

   OSSleepThreadNoLock(&alarm->threadQueue);
   OSUninterruptibleSpinLock_Release(gAlarmLock);
   OSRescheduleNoLock();

   OSUninterruptibleSpinLock_Acquire(gAlarmLock);

   if (alarm->state != OSAlarmState::Cancelled) {
      result = TRUE;
   }

   OSUninterruptibleSpinLock_Release(gAlarmLock);
   OSUnlockScheduler();
   return result;
}


/**
* Internal alarm handler.
*
* Resets the alarm state.
* Calls the users callback.
* Wakes up any threads waiting on the alarm.
*/
static void
triggerAlarmNoLock(OSAlarm *alarm, OSContext *context)
{
   alarm->context = context;

   if (alarm->period) {
      alarm->nextFire = OSGetTime() + alarm->period;
      alarm->state = OSAlarmState::Set;
   } else {
      alarm->nextFire = 0;
      alarm->state = OSAlarmState::None;
      OSEraseFromQueue<OSAlarmQueue>(alarm->alarmQueue, alarm);
   }

   if (alarm->callback) {
      OSUninterruptibleSpinLock_Release(gAlarmLock);
      alarm->callback(alarm, context);
      OSUninterruptibleSpinLock_Acquire(gAlarmLock);
   }

   OSWakeupThreadNoLock(&alarm->threadQueue);
}


namespace coreinit
{

namespace internal
{

/**
 * Internal check to see if any alarms are ready to be triggered.
 *
 * Updates the processor internal interrupt timer to trigger for the next ready alarm.
 */
void
checkAlarms(uint32_t core, OSContext *context)
{
   auto queue = gAlarmQueue[core];
   auto now = OSGetTime();
   auto next = std::chrono::time_point<std::chrono::system_clock>::max();

   OSLockScheduler();
   OSUninterruptibleSpinLock_Acquire(gAlarmLock);

   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      // Trigger alarm if it is time
      if (alarm->nextFire <= now && alarm->state != OSAlarmState::Cancelled) {
         triggerAlarmNoLock(alarm, context);
      }

      // Set next timer if alarm is set
      if (alarm->state == OSAlarmState::Set && alarm->nextFire) {
         auto nextFire = OSTimeToChrono(alarm->nextFire);

         if (nextFire < next) {
            next = nextFire;
         }
      }

      alarm = nextAlarm;
   }

   OSUninterruptibleSpinLock_Release(gAlarmLock);
   OSUnlockScheduler();
   gProcessor.setInterruptTimer(core, next);
}

} // namespace internal

} // namespace coreinit


void
CoreInit::registerAlarmFunctions()
{
   RegisterKernelFunction(OSCancelAlarm);
   RegisterKernelFunction(OSCancelAlarms);
   RegisterKernelFunction(OSCreateAlarm);
   RegisterKernelFunction(OSCreateAlarmEx);
   RegisterKernelFunction(OSGetAlarmUserData);
   RegisterKernelFunction(OSSetAlarm);
   RegisterKernelFunction(OSSetPeriodicAlarm);
   RegisterKernelFunction(OSSetAlarmTag);
   RegisterKernelFunction(OSSetAlarmUserData);
   RegisterKernelFunction(OSWaitAlarm);
}


void
CoreInit::initialiseAlarm()
{
   gAlarmLock = OSAllocFromSystem<OSSpinLock>();

   for (auto i = 0u; i < CoreCount; ++i) {
      gAlarmQueue[i] = OSAllocFromSystem<OSAlarmQueue>();
      OSInitAlarmQueue(gAlarmQueue[i]);
   }
}
