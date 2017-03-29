#include "coreinit.h"
#include "coreinit_alarm.h"
#include "coreinit_core.h"
#include "coreinit_spinlock.h"
#include "coreinit_scheduler.h"
#include "coreinit_thread.h"
#include "coreinit_memheap.h"
#include "coreinit_time.h"
#include "coreinit_internal_queue.h"
#include "coreinit_internal_idlock.h"
#include "ppcutils/wfunc_call.h"

#include <array>
#include <common/decaf_assert.h>
#include <libcpu/cpu.h>

namespace coreinit
{

namespace internal
{

using AlarmQueue = Queue<OSAlarmQueue, OSAlarmLink, OSAlarm, &OSAlarm::link>;
void updateCpuAlarmNoALock();

} // namespace internal

static OSThreadEntryPointFn
sAlarmCallbackThreadEntryPoint;

const uint32_t
OSAlarm::Tag;

const uint32_t
OSAlarmQueue::Tag;

static internal::IdLock
sAlarmLock;

static std::array<OSAlarmQueue *, CoreCount>
sAlarmQueue;

static std::array<OSAlarmQueue *, CoreCount>
sAlarmCallbackQueue;

static std::array<OSThread *, CoreCount>
sAlarmCallbackThread;

static std::array<OSThreadQueue *, CoreCount>
sAlarmCallbackThreadQueue;

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
   // We are technically supposed to try and remove the alarm
   //  from the callback queue as well.
   if (alarm->state != OSAlarmState::Set) {
      return FALSE;
   }

   alarm->state = OSAlarmState::None;
   alarm->nextFire = 0;
   alarm->period = 0;

   if (alarm->alarmQueue) {
      internal::AlarmQueue::erase(alarm->alarmQueue, alarm);
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
OSCreateAlarmEx(OSAlarm *alarm,
                const char *name)
{
   // Holding the alarm here is neccessary since its valid to call
   // Create on an already active alarm, as long as its not set.
   internal::acquireIdLock(sAlarmLock, alarm);

   memset(alarm, 0, sizeof(OSAlarm));
   alarm->tag = OSAlarm::Tag;
   alarm->name = name;
   OSInitThreadQueueEx(&alarm->threadQueue, alarm);

   internal::releaseIdLock(sAlarmLock, alarm);
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
OSInitAlarmQueueEx(OSAlarmQueue *queue,
                   const char *name)
{
   memset(queue, 0, sizeof(OSAlarmQueue));
   queue->tag = OSAlarmQueue::Tag;
   queue->name = name;
}


/**
 * Set a one shot alarm to perform a callback after an amount of time.
 */
BOOL
OSSetAlarm(OSAlarm *alarm,
           OSTime time,
           AlarmCallback callback)
{
   return OSSetPeriodicAlarm(alarm, OSGetTime() + time, 0, callback);
}


/**
 * Set a repeated alarm to execute a callback every interval from start.
 *
 * \param alarm
 * The alarm to set.
 *
 * \param start
 * The duration until the alarm should first be triggered.
 *
 * \param interval
 * The interval between triggers after the first trigger.
 *
 * \param callback
 * The alarm callback to call when the alarm is triggered.
 *
 * \return
 * Returns TRUE if the alarm was succesfully set, FALSE otherwise.
 */
BOOL
OSSetPeriodicAlarm(OSAlarm *alarm,
                   OSTime start,
                   OSTime interval,
                   AlarmCallback callback)
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
      internal::AlarmQueue::erase(alarm->alarmQueue, alarm);
      alarm->alarmQueue = nullptr;
   }

   // Add to this core's alarm queue
   auto core = OSGetCoreId();
   auto queue = sAlarmQueue[core];
   alarm->alarmQueue = queue;
   internal::AlarmQueue::append(queue, alarm);

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
OSSetAlarmTag(OSAlarm *alarm,
              uint32_t group)
{
   internal::acquireIdLock(sAlarmLock, alarm);
   alarm->group = group;
   internal::releaseIdLock(sAlarmLock, alarm);
}


/**
 * Set alarm user data which is returned by OSGetAlarmUserData.
 */
void
OSSetAlarmUserData(OSAlarm *alarm,
                   void *data)
{
   internal::acquireIdLock(sAlarmLock, alarm);
   alarm->userData = data;
   internal::releaseIdLock(sAlarmLock, alarm);
}


/**
 * Sleep the current thread until the alarm has been triggered or cancelled.
 *
 * \return
 * Returns TRUE if alarm expired, FALSE if alarm cancelled
 */
BOOL
OSWaitAlarm(OSAlarm *alarm)
{
   internal::lockScheduler();
   internal::acquireIdLock(sAlarmLock, alarm);

   decaf_check(alarm);
   decaf_check(alarm->tag == OSAlarm::Tag);

   if (alarm->state != OSAlarmState::Set) {
      internal::releaseIdLock(sAlarmLock, alarm);
      internal::unlockScheduler();
      return FALSE;
   }

   OSGetCurrentThread()->alarmCancelled = false;
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

uint32_t
AlarmCallbackThreadEntry(uint32_t core_id,
                         void *arg2)
{
   auto queue = sAlarmQueue[core_id];
   auto cbQueue = sAlarmCallbackQueue[core_id];
   auto threadQueue = sAlarmCallbackThreadQueue[core_id];

   while (true) {
      internal::lockScheduler();
      internal::acquireIdLock(sAlarmLock);

      OSAlarm *alarm = internal::AlarmQueue::popFront(cbQueue);
      if (alarm == nullptr) {
         // No alarms currently pending for callback
         internal::sleepThreadNoLock(threadQueue);
         internal::releaseIdLock(sAlarmLock);

         internal::rescheduleSelfNoLock();
         internal::unlockScheduler();
         continue;
      }

      if (alarm->period) {
         alarm->nextFire = alarm->nextFire + alarm->period;
         alarm->state = OSAlarmState::Set;
         internal::AlarmQueue::append(queue, alarm);
         alarm->alarmQueue = queue;
         internal::updateCpuAlarmNoALock();
      }

      internal::releaseIdLock(sAlarmLock);
      internal::unlockScheduler();

      if (alarm->callback) {
         alarm->callback(alarm, alarm->context);
      }
   }
   return 0;
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

   RegisterInternalFunction(AlarmCallbackThreadEntry, sAlarmCallbackThreadEntryPoint);
   RegisterInternalData(sAlarmQueue);
   RegisterInternalData(sAlarmCallbackQueue);
   RegisterInternalData(sAlarmCallbackThreadQueue);
   RegisterInternalData(sAlarmCallbackThread);
}

void
Module::initialiseAlarm()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      OSInitAlarmQueue(sAlarmQueue[i]);
      OSInitAlarmQueue(sAlarmCallbackQueue[i]);
      OSInitThreadQueue(sAlarmCallbackThreadQueue[i]);
   }
}

namespace internal
{

void
startAlarmCallbackThreads()
{
   for (auto i = 0u; i < CoreCount; ++i) {
      auto &thread = sAlarmCallbackThread[i];
      auto stackSize = 16 * 1024;
      auto stack = reinterpret_cast<uint8_t*>(coreinit::internal::sysAlloc(stackSize, 8));
      auto name = coreinit::internal::sysStrDup(fmt::format("Alarm Thread {}", i));

      OSCreateThread(thread, sAlarmCallbackThreadEntryPoint, i, nullptr,
         reinterpret_cast<be_val<uint32_t>*>(stack + stackSize), stackSize, -1,
         static_cast<OSThreadAttributes>(1 << i));
      OSSetThreadName(thread, name);
      OSResumeThread(thread);
   }
}

BOOL
setAlarmInternal(OSAlarm *alarm, OSTime time, AlarmCallback callback, void *userData)
{
   alarm->group = 0xFFFFFFFF;
   alarm->userData = userData;
   return OSSetAlarm(alarm, time, callback);
}

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
   auto next = std::chrono::steady_clock::time_point::max();

   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      // Update next if its not past yet
      if (alarm->state == OSAlarmState::Set && alarm->nextFire) {
         auto nextFire = cpu::tbToTimePoint(alarm->nextFire - internal::getBaseTime());

         if (nextFire < next) {
            next = nextFire;
         }
      }

      alarm = nextAlarm;
   }

   cpu::this_core::setNextAlarm(next);
}

void
handleAlarmInterrupt(OSContext *context)
{
   auto core_id = cpu::this_core::id();
   auto queue = sAlarmQueue[core_id];
   auto cbQueue = sAlarmCallbackQueue[core_id];
   auto cbThreadQueue = sAlarmCallbackThreadQueue[core_id];

   auto now = OSGetTime();
   auto next = std::chrono::time_point<std::chrono::system_clock>::max();
   bool callbacksNeeded = false;

   internal::lockScheduler();
   acquireIdLock(sAlarmLock);

   for (OSAlarm *alarm = queue->head; alarm; ) {
      auto nextAlarm = alarm->link.next;

      // Expire it if its past its nextFire time
      if (alarm->nextFire <= now) {
         decaf_check(alarm->state == OSAlarmState::Set);

         internal::AlarmQueue::erase(queue, alarm);
         alarm->alarmQueue = nullptr;

         alarm->state = OSAlarmState::Expired;
         alarm->context = context;

         if (alarm->threadQueue.head) {
            wakeupThreadNoLock(&alarm->threadQueue);
            rescheduleOtherCoreNoLock();
         }

         if (alarm->group == 0xFFFFFFFF) {
            // System-internal alarm
            if (alarm->callback) {
               auto originalMask = cpu::this_core::setInterruptMask(0);
               alarm->callback(alarm, context);
               cpu::this_core::setInterruptMask(originalMask);
            }
         } else {
            internal::AlarmQueue::append(cbQueue, alarm);
            alarm->alarmQueue = cbQueue;

            wakeupThreadNoLock(cbThreadQueue);
         }
      }

      alarm = nextAlarm;
   }

   internal::updateCpuAlarmNoALock();

   releaseIdLock(sAlarmLock);
   internal::unlockScheduler();
}

} // namespace internal

} // namespace coreinit
