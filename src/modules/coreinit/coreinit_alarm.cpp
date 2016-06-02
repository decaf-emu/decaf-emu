#include <array>
#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_spinlock.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "coreinit_queue.h"
#include "coreinit_internal_idlock.h"
#include "utils/wfunc_call.h"
#include "cpu/cpu.h"

namespace coreinit
{

namespace internal
{
   void updateCpuAlarmNoALock();
}

const uint32_t
OSAlarm::Tag;

const uint32_t
OSAlarmQueue::Tag;

static internal::IdLock
sAlarmLock;

static std::array<OSAlarmQueue *, CoreCount>
sAlarmQueue;

/**
 * Internal alarm cancel.
 *
 * Reset the alarm state to cancelled.
 * Wakes up all threads waiting on the alarm.
 * Removes the alarm from any queue it is in.
 */
static BOOL
cancelAlarmNoAlarmLock(OSAlarm *alarm)
{
   if (alarm->state != OSAlarmState::Set) {
      return FALSE;
   }

   alarm->state = OSAlarmState::None;
   alarm->nextFire = 0;
   alarm->period = 0;

   if (alarm->alarmQueue) {
      OSEraseFromQueue<OSAlarmQueue>(alarm->alarmQueue, alarm);
      alarm->alarmQueue = nullptr;
   }

   return TRUE;
}


/**
 * Cancel an alarm.
 */
BOOL
OSCancelAlarm(OSAlarm *alarm)
{
   // First cancel the alarm whilst holding alarm lock
   internal::acquireIdLock(sAlarmLock, alarm);
   auto result = cancelAlarmNoAlarmLock(alarm);
   internal::releaseIdLock(sAlarmLock, alarm);

   if (!result) {
      return FALSE;
   }

   // Now wakeup any waiting threads
   internal::lockScheduler();

   for (auto thread = alarm->threadQueue.head; thread; thread = thread->link.next) {
      thread->alarmCancelled = TRUE;
   }

   internal::wakeupThreadNoLock(&alarm->threadQueue);
   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
   return TRUE;
}


/**
 * Cancel all alarms which have a matching tag.
 */
void
OSCancelAlarms(uint32_t group)
{
   internal::lockScheduler();
   internal::acquireIdLock(sAlarmLock);

   for (auto i = 0u; i < 3; ++i) {
      auto queue = sAlarmQueue[i];

      for (OSAlarm *alarm = queue->head; alarm; ) {
         auto next = alarm->link.next;

         if (alarm->group == group) {
            if (cancelAlarmNoAlarmLock(alarm)) {
               for (auto thread = alarm->threadQueue.head; thread; thread = thread->link.next) {
                  thread->alarmCancelled = TRUE;
               }

               internal::wakeupThreadNoLock(&alarm->threadQueue);
            }
         }

         alarm = next;
      }
   }

   internal::releaseIdLock(sAlarmLock);

   internal::rescheduleAllCoreNoLock();
   internal::unlockScheduler();
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
   OSInitAlarmQueueEx(queue, nullptr);
}


/**
 * Initialise an alarm queue structure with a name
 */
void
OSInitAlarmQueueEx(OSAlarmQueue *queue, const char *name)
{
   memset(queue, 0, sizeof(OSAlarmQueue));
   queue->tag = OSAlarmQueue::Tag;
   queue->name = name;
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
   internal::acquireIdLock(sAlarmLock, alarm);

   // Set alarm
   alarm->nextFire = start;
   alarm->callback = callback;
   alarm->period = interval;
   alarm->context = nullptr;
   alarm->state = OSAlarmState::Set;

   // Erase from old alarm queue
   if (alarm->alarmQueue) {
      OSEraseFromQueue(static_cast<OSAlarmQueue*>(alarm->alarmQueue), alarm);
      alarm->alarmQueue = nullptr;
   }

   // Add to this core's alarm queue
   auto core = OSGetCoreId();
   auto queue = sAlarmQueue[core];
   alarm->alarmQueue = queue;
   OSAppendQueue(queue, alarm);

   // Set the interrupt timer in processor
   // TODO: Store the last set CPU alarm time, and simply check this
   // alarm against that time to make finding the soonest alarm cheaper.
   internal::updateCpuAlarmNoALock();

   internal::releaseIdLock(sAlarmLock, alarm);
   return TRUE;
}


/**
 * Set an alarm tag which is used in OSCancelAlarms for bulk cancellation.
 */
void
OSSetAlarmTag(OSAlarm *alarm, uint32_t group)
{
   internal::acquireIdLock(sAlarmLock, alarm);
   alarm->group = group;
   internal::releaseIdLock(sAlarmLock, alarm);
}


/**
 * Set alarm user data which is returned by OSGetAlarmUserData.
 */
void
OSSetAlarmUserData(OSAlarm *alarm, void *data)
{
   internal::acquireIdLock(sAlarmLock, alarm);
   alarm->userData = data;
   internal::releaseIdLock(sAlarmLock, alarm);
}


/**
 * Sleep the current thread until the alarm has been triggered or cancelled.
 *
 * \return Returns TRUE if alarm expired, FALSE if alarm cancelled
 */
BOOL
OSWaitAlarm(OSAlarm *alarm)
{
   internal::lockScheduler();
   internal::acquireIdLock(sAlarmLock, alarm);

   assert(alarm);
   assert(alarm->tag == OSAlarm::Tag);

   if (alarm->state != OSAlarmState::Set) {
      internal::releaseIdLock(sAlarmLock, alarm);
      internal::unlockScheduler();
      return FALSE;
   }

   internal::sleepThreadNoLock(&alarm->threadQueue);
   internal::releaseIdLock(sAlarmLock, alarm);

   internal::rescheduleSelfNoLock();

   auto cancelled = OSGetCurrentThread()->alarmCancelled;
   internal::unlockScheduler();

   if (cancelled) {
      return FALSE;
   } else {
      return TRUE;
   }
}


void
Module::registerAlarmFunctions()
{
   RegisterKernelFunction(OSCancelAlarm);
   RegisterKernelFunction(OSCancelAlarms);
   RegisterKernelFunction(OSCreateAlarm);
   RegisterKernelFunction(OSCreateAlarmEx);
   RegisterKernelFunction(OSGetAlarmUserData);
   RegisterKernelFunction(OSInitAlarmQueue);
   RegisterKernelFunction(OSInitAlarmQueueEx);
   RegisterKernelFunction(OSSetAlarm);
   RegisterKernelFunction(OSSetPeriodicAlarm);
   RegisterKernelFunction(OSSetAlarmTag);
   RegisterKernelFunction(OSSetAlarmUserData);
   RegisterKernelFunction(OSWaitAlarm);
}

void
Module::initialiseAlarm()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      sAlarmQueue[i] = internal::sysAlloc<OSAlarmQueue>();
      OSInitAlarmQueue(sAlarmQueue[i]);
   }
}

namespace internal
{

bool
cancelAlarm(OSAlarm *alarm)
{
   internal::acquireIdLock(sAlarmLock, alarm);
   auto result = cancelAlarmNoAlarmLock(alarm);
   internal::releaseIdLock(sAlarmLock, alarm);
   return result == TRUE;
}

void
updateCpuAlarmNoALock()
{
   auto queue = sAlarmQueue[cpu::this_core::id()];
   auto next = std::chrono::time_point<std::chrono::system_clock>::max();

   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      // Update next if its not past yet
      if (alarm->state == OSAlarmState::Set && alarm->nextFire) {
         auto nextFire = internal::toTimepoint(alarm->nextFire);

         if (nextFire < next) {
            next = nextFire;
         }
      }

      alarm = nextAlarm;
   }

   cpu::this_core::set_next_alarm(next);
}

void
handleAlarmInterrupt(OSContext *context)
{
   auto core_id = cpu::this_core::id();
   auto queue = sAlarmQueue[core_id];
   auto now = OSGetTime();
   auto next = std::chrono::time_point<std::chrono::system_clock>::max();
   bool alarmExpired = false;

   // TODO: Brett check my logic!

   acquireIdLock(sAlarmLock);

   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      // Expire it if its past its nextFire time
      if (alarm->nextFire <= now && alarm->state == OSAlarmState::Set) {
         alarm->state = OSAlarmState::Expired;
         alarm->context = context;
         alarmExpired = true;
      }

      // Update next if its not past yet
      if (alarm->state == OSAlarmState::Set && alarm->nextFire) {
         auto nextFire = internal::toTimepoint(alarm->nextFire);

         if (nextFire < next) {
            next = nextFire;
         }
      }

      alarm = nextAlarm;
   }

   cpu::this_core::set_next_alarm(next);

   releaseIdLock(sAlarmLock);

   if (alarmExpired) {
      internal::lockScheduler();
      internal::signalIoThreadNoLock(core_id);
      internal::unlockScheduler();
   }
}

/**
 * Internal check to see if any alarms are ready to be triggered.
 *
 * Updates the processor internal interrupt timer to trigger for the next ready alarm.
 */
void
checkAlarms(uint32_t core_id)
{
   auto queue = sAlarmQueue[core_id];
   bool alarmsScheduled = false;

   // TODO: Brett check my logic!

   acquireIdLock(sAlarmLock);

   // Trigger all pending alarms
   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      if (alarm->state == OSAlarmState::Expired)
      {
         if (alarm->period) {
            alarm->nextFire = alarm->nextFire + alarm->period;
            alarm->state = OSAlarmState::Set;
            alarmsScheduled = true;
         } else {
            alarm->nextFire = 0;
            alarm->alarmQueue = nullptr;
            OSEraseFromQueue<OSAlarmQueue>(queue, alarm);
         }

         // TODO: This is technically not safe, as someone could mess with
         // this alarm while we are trying to dispatch callbacks and wake things.
         releaseIdLock(sAlarmLock);

         if (alarm->callback) {
            alarm->callback(alarm, alarm->context);
         }

         OSWakeupThread(&alarm->threadQueue);

         acquireIdLock(sAlarmLock);
      }

      alarm = nextAlarm;
   }

   // If any alarms were rescheduled, we need to update the CPU timer.
   if (alarmsScheduled) {
      internal::updateCpuAlarmNoALock();
   }

   releaseIdLock(sAlarmLock);
}

} // namespace internal

} // namespace coreinit
