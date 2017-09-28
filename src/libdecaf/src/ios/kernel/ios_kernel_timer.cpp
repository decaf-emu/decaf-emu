#include "ios_kernel_timer.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_process.h"
#include "ios_kernel_thread.h"

#include "ios/ios_stackobject.h"

#include <chrono>

namespace ios::kernel
{

constexpr auto NumMessages = 1u;
constexpr auto ThreadStackSize = 0x400u;
constexpr auto ThreadPriority = 125u;

struct TimerData
{
   be2_val<ThreadId> threadId;
   be2_val<MessageQueueId> messageQueueId;
   be2_array<Message, NumMessages> messageBuffer;
   be2_array<uint8_t, ThreadStackSize> threadStack;
   be2_struct<TimerManager> timerManager;
};

static phys_ptr<TimerData>
sData;

static std::chrono::time_point<std::chrono::steady_clock>
sStartupTime;

namespace internal
{

uint32_t
startTimer(phys_ptr<Timer> timer);

void
setAlarm(TimerTicks when);

} // namespace internal

TimerTicks
IOS_GetTimerTicks()
{
   return internal::timeToTicks(std::chrono::steady_clock::now());
}

Error
IOS_CreateTimer(std::chrono::microseconds delay,
                std::chrono::microseconds period,
                MessageQueueId queue,
                Message message)
{
   auto &timerManager = sData->timerManager;

   // Verify the message queue exists.
   auto error = internal::getMessageQueue(queue, nullptr);
   if (error < Error::OK) {
      return error;
   }

   // Check the process has not exceeded its maximum number of processes.
   auto pid = internal::getCurrentProcessId();
   if (timerManager.numProcessTimers[pid] >= MaxNumTimersPerProcess) {
      return Error::Max;
   }

   if (timerManager.firstFreeIdx < 0) {
      return Error::FailAlloc;
   }

   auto timerIdx = timerManager.firstFreeIdx;
   auto &timer = timerManager.timers[timerIdx];
   auto nextFreeIdx = timer.nextTimerIdx;

   if (nextFreeIdx < 0) {
      timerManager.firstFreeIdx = int16_t { -1 };
      timerManager.lastFreeIdx = int16_t { -1 };
   } else {
      auto &nextTimer = timerManager.timers[nextFreeIdx];
      nextTimer.prevTimerIdx = int16_t { -1 };
      timerManager.firstFreeIdx = nextFreeIdx;
   }

   timerManager.totalCreatedTimers++;

   timer.uid = timerIdx | ((timerManager.totalCreatedTimers << 12) & 0x7FFFFFFF);
   timer.state = TimerState::Ready;
   timer.nextTriggerTime = TimerTicks { 0 };
   timer.period = static_cast<TimeMicroseconds>(period.count());
   timer.queueId = queue;
   timer.message = message;
   timer.processId = pid;
   timer.prevTimerIdx = int16_t { -1 };
   timer.nextTimerIdx = int16_t { -1 };

   timerManager.numRegistered++;
   if (timerManager.numRegistered > timerManager.mostRegistered) {
      timerManager.mostRegistered = timerManager.numRegistered;
   }

   if (delay.count() || period.count()) {
      timer.nextTriggerTime = IOS_GetTimerTicks() + internal::durationToTicks(delay);
      if (internal::startTimer(phys_addrof(timer)) == 0) {
         internal::setAlarm(timer.nextTriggerTime);
      }
   }

   return Error::OK;
}

Error
IOS_DestroyTimer(TimerId timerId)
{
   phys_ptr<Timer> timer;
   auto &timerManager = sData->timerManager;

   auto error = internal::getTimer(timerId, &timer);
   if (error < Error::OK) {
      return error;
   }

   if (timer->state == TimerState::Running) {
      error = internal::stopTimer(timer);
      if (error < Error::OK) {
         return error;
      }
   }

   auto pid = internal::getCurrentProcessId();
   timerManager.numProcessTimers[pid]--;

   timer->state = TimerState::Free;
   timer->uid = TimerId { -4 };
   timer->processId = ProcessId { -4 };
   timer->queueId = MessageQueueId { -4 };
   timer->message = Message { 0 };
   timer->period = 0u;
   timer->nextTriggerTime = TimerTicks{ 0 };

   if (timerManager.lastFreeIdx < 0) {
      timerManager.firstFreeIdx = timer->index;
      timerManager.lastFreeIdx = timer->index;

      timer->nextTimerIdx = int16_t { -1 };
      timer->prevTimerIdx = int16_t { -1 };
   } else {
      auto &lastFreeTimer = timerManager.timers[timerManager.lastFreeIdx];
      lastFreeTimer.nextTimerIdx = timer->index;

      timer->prevTimerIdx = timerManager.lastFreeIdx;
      timer->nextTimerIdx = int16_t { -1 };

      timerManager.lastFreeIdx = timer->index;
   }

   timerManager.numRegistered--;
   return Error::OK;
}

Error
IOS_StopTimer(TimerId timerId)
{
   phys_ptr<Timer> timer;
   auto error = internal::getTimer(timerId, &timer);
   if (error < Error::OK) {
      return error;
   }

   if (timer->state == TimerState::Running) {
      error = internal::stopTimer(timer);
   } else {
      timer->state = TimerState::Stopped;
      error = Error::Expired;
   }

   return error;
}

Error
IOS_RestartTimer(TimerId timerId,
                 std::chrono::microseconds delay,
                 std::chrono::microseconds period)
{
   phys_ptr<Timer> timer;
   auto &timerManager = sData->timerManager;

   auto error = internal::getTimer(timerId, &timer);
   if (error < Error::OK) {
      return error;
   }

   if (timer->state == TimerState::Running) {
      error = internal::stopTimer(timer);
      if (error < Error::OK) {
         return error;
      }
   }

   timer->nextTriggerTime = TimerTicks { 0 };
   timer->period = static_cast<TimeMicroseconds>(period.count());

   if (delay.count() || period.count()) {
      timer->nextTriggerTime = IOS_GetTimerTicks() + internal::durationToTicks(delay);
      if (internal::startTimer(timer) == 0) {
         internal::setAlarm(timer->nextTriggerTime);
      }
   }

   return error;
}

namespace internal
{

static Error
timerThreadEntry(phys_ptr<void> /*context*/)
{
   StackObject<Message> message;

   while (true) {
      auto error = IOS_ReceiveMessage(sData->messageQueueId,
                                      message,
                                      MessageFlags::NonBlocking);
      if (error < Error::OK) {
         return error;
      }

      auto now = IOS_GetTimerTicks();
      while (true) {
         // Trigger all ready timers

      }
   }
}

void
setAlarm(TimerTicks when)
{
   // TODO: Hook this up to a timer which triggers a timer interrupt
}

TimerTicks
timeToTicks(std::chrono::steady_clock::time_point time)
{
   auto dt = time - sStartupTime;
   return std::chrono::duration_cast<std::chrono::nanoseconds>(dt).count();
}

Error
startTimerThread()
{
   sStartupTime = std::chrono::steady_clock::now();

   // TODO: Alloc sData
   // TODO: Initialise sData as per kernel::initTimerThread

   // Create message queue
   auto error = IOS_CreateMessageQueue(phys_addrof(sData->messageBuffer), sData->messageBuffer.size());
   if (error < Error::OK) {
      return error;
   }
   sData->messageQueueId = static_cast<MessageQueueId>(error);

   // Create thread
   error = kernel::IOS_CreateThread(&timerThreadEntry, nullptr,
                                    phys_addrof(sData->threadStack) + sData->threadStack.size(),
                                    sData->threadStack.size(),
                                    ThreadPriority,
                                    kernel::ThreadFlags::Detached);
   if (error < Error::OK) {
      kernel::IOS_DestroyMessageQueue(sData->messageQueueId);
      return error;
   }

   sData->threadId = static_cast<kernel::ThreadId>(error);
   return kernel::IOS_StartThread(sData->threadId);
}

Error
getTimer(TimerId id,
         phys_ptr<Timer> *outTimer)
{
   auto index = id & 0xFFF;

   if (id < 0 || index >= sData->timerManager.timers.size()) {
      return Error::Invalid;
   }

   if (sData->timerManager.timers[index].uid != id) {
      return Error::NoExists;
   }

   *outTimer = phys_addrof(sData->timerManager.timers[index]);
   return Error::OK;
}

/**
 * Put the timer into the running timer list.
 *
 * \return
 * Returns the position of the timer in the queue, if the position is 0 then
 * the new timer will be the first one to trigger in which case the caller
 * is responsbile for updating the next interrupt time.
 */
uint32_t
startTimer(phys_ptr<Timer> timer)
{
   auto &timerManager = sData->timerManager;
   auto timerQueuePosition = 0u;

   // Insert timer into the running timer list ordered by nextTriggerTime.
   if (timerManager.firstRunningTimerIdx < 0) {
      timerManager.firstRunningTimerIdx = timer->index;
      timerManager.lastRunningTimerIdx = timer->index;

      timer->prevTimerIdx = int16_t { -1 };
      timer->nextTimerIdx = int16_t { -1 };
   } else {
      auto prevTimerIdx = int16_t { -1 };
      auto nextTimerIdx = int16_t { -1 };

      for (auto i = timerManager.firstRunningTimerIdx; i < timerManager.lastRunningTimerIdx; ++i) {
         if (timerManager.timers[i].nextTriggerTime >= timer->nextTriggerTime) {
            break;
         }

         prevTimerIdx = i;
      }

      if (prevTimerIdx < 0) {
         nextTimerIdx = timerManager.firstRunningTimerIdx;
         timerManager.firstRunningTimerIdx = timer->index;
      } else {
         auto &prevTimer = timerManager.timers[prevTimerIdx];
         nextTimerIdx = prevTimer.nextTimerIdx;
         prevTimer.nextTimerIdx = timer->index;
      }

      if (nextTimerIdx < 0) {
         decaf_check(timerManager.lastRunningTimerIdx == prevTimerIdx);
         timerManager.lastRunningTimerIdx = timer->index;
      } else {
         auto &nextTimer = timerManager.timers[nextTimerIdx];
         nextTimer.prevTimerIdx = timer->index;
      }

      timer->prevTimerIdx = prevTimerIdx;
      timer->nextTimerIdx = nextTimerIdx;
   }

   timer->state = TimerState::Running;
   timerManager.numRunningTimers++;
   return timerManager.firstRunningTimerIdx == timer->index;
}

Error
stopTimer(phys_ptr<Timer> timer)
{
   auto &timerManager = sData->timerManager;
   auto prevTimerIdx = timer->prevTimerIdx;
   auto nextTimerIdx = timer->nextTimerIdx;

   if (prevTimerIdx < 0) {
      decaf_check(timerManager.firstRunningTimerIdx == timer->index);
      timerManager.firstRunningTimerIdx = nextTimerIdx;
   } else {
      auto &prevTimer = timerManager.timers[prevTimerIdx];
      prevTimer.nextTimerIdx = nextTimerIdx;
   }

   if (nextTimerIdx < 0) {
      decaf_check(timerManager.lastRunningTimerIdx == timer->index);
      timerManager.lastRunningTimerIdx = prevTimerIdx;
   } else {
      auto &nextTimer = timerManager.timers[nextTimerIdx];
      nextTimer.prevTimerIdx = prevTimerIdx;
   }

   timer->state = TimerState::Stopped;
   timer->prevTimerIdx = int16_t { -1 };
   timer->nextTimerIdx = int16_t { -1 };
   timerManager.numRunningTimers--;
   return Error::OK;
}

} // namespace internal

} // namespace ios::kernel
