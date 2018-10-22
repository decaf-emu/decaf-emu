#pragma once
#include "ios_kernel_enum.h"
#include "ios_kernel_messagequeue.h"
#include "ios_kernel_process.h"
#include "ios/ios_enum.h"

#include <chrono>
#include <cstdint>
#include <libcpu/be2_struct.h>
#include <common/structsize.h>

namespace ios::kernel
{

#pragma pack(push, 1)

static constexpr auto MaxNumTimers = 256u;
static constexpr auto MaxNumTimersPerProcess = 64u;

using TimerId = int32_t;
using TimerTicks = uint64_t;
using TimeMicroseconds32 = uint32_t;
using TimeMicroseconds64 = uint64_t;

struct Timer
{
   be2_val<TimerId> uid;
   be2_val<TimerState> state;
   be2_val<TimerTicks> nextTriggerTime;
   be2_val<TimeMicroseconds32> period;
   be2_val<MessageQueueId> queueId;
   be2_val<Message> message;
   be2_val<ProcessId> processId;

   //! This timers index in the TimerManager timers list.
   be2_val<int16_t> index;

   //! If state == Free this is the index of the next free timer.
   //! If state == Running this is the index of the next running timer.
   be2_val<int16_t> nextTimerIdx;

   //! If state == Free this is the index of the previous free timer.
   //! If state == Running this is the index of the previous running timer.
   be2_val<int16_t> prevTimerIdx;

   be2_val<uint16_t> unk0x26;
};
CHECK_OFFSET(Timer, 0x00, uid);
CHECK_OFFSET(Timer, 0x04, state);
CHECK_OFFSET(Timer, 0x08, nextTriggerTime);
CHECK_OFFSET(Timer, 0x10, period);
CHECK_OFFSET(Timer, 0x14, queueId);
CHECK_OFFSET(Timer, 0x18, message);
CHECK_OFFSET(Timer, 0x1C, processId);
CHECK_OFFSET(Timer, 0x20, index);
CHECK_OFFSET(Timer, 0x22, nextTimerIdx);
CHECK_OFFSET(Timer, 0x24, prevTimerIdx);
CHECK_OFFSET(Timer, 0x26, unk0x26);
CHECK_SIZE(Timer, 0x28);

struct TimerManager
{
   //! Total number of timers that have been created.
   be2_val<uint32_t> totalCreatedTimers;

   //! Number of timers each process has
   be2_array<uint16_t, NumIosProcess> numProcessTimers;

   //! Index of the first running Timer, ordered by nextTriggerTime.
   be2_val<int16_t> firstRunningTimerIdx;

   //! Index of the last running Timer, ordered by nextTriggerTime.
   be2_val<int16_t> lastRunningTimerIdx;

   //! Number of actively running timers.
   be2_val<uint16_t> numRunningTimers;

   //! Index of the first free Timer.
   be2_val<int16_t> firstFreeIdx;

   //! Index of the last free Timer.
   be2_val<int16_t> lastFreeIdx;

   //! Number of registered Timers.
   be2_val<uint16_t> numRegistered;

   //! Highest number of registered Timers at one time.
   be2_val<uint16_t> mostRegistered;

   be2_val<uint16_t> unk0x2E;

   be2_array<Timer, MaxNumTimers> timers;
};
CHECK_OFFSET(TimerManager, 0x00, totalCreatedTimers);
CHECK_OFFSET(TimerManager, 0x04, numProcessTimers);
CHECK_OFFSET(TimerManager, 0x20, firstRunningTimerIdx);
CHECK_OFFSET(TimerManager, 0x22, lastRunningTimerIdx);
CHECK_OFFSET(TimerManager, 0x24, numRunningTimers);
CHECK_OFFSET(TimerManager, 0x26, firstFreeIdx);
CHECK_OFFSET(TimerManager, 0x28, lastFreeIdx);
CHECK_OFFSET(TimerManager, 0x2A, numRegistered);
CHECK_OFFSET(TimerManager, 0x2C, mostRegistered);
CHECK_OFFSET(TimerManager, 0x2E, unk0x2E);
CHECK_OFFSET(TimerManager, 0x30, timers);

#pragma pack(pop)

Error
IOS_GetUpTime64(phys_ptr<TimerTicks> outTime);

Error
IOS_CreateTimer(std::chrono::microseconds delay,
                std::chrono::microseconds period,
                MessageQueueId queue,
                Message message);

Error
IOS_DestroyTimer(TimerId timerId);

Error
IOS_StopTimer(TimerId timerId);

Error
IOS_RestartTimer(TimerId timerId,
                 std::chrono::microseconds delay,
                 std::chrono::microseconds period);

namespace internal
{

Error
getTimer(TimerId id,
         phys_ptr<Timer> *outTimer);

Error
stopTimer(phys_ptr<Timer> timer);

TimerTicks
timeToTicks(std::chrono::steady_clock::time_point time);

template<class Rep, class Period>
TimerTicks
durationToTicks(std::chrono::duration<Rep, Period> duration)
{
   return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

Error
startTimerThread();

void
initialiseStaticTimerData();

TimerTicks
getUpTime64();

} // namespace internal

} // namespace ios::kernel
