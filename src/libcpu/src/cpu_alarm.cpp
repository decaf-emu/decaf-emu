#include "cpu.h"
#include "cpu_alarm.h"
#include "cpu_breakpoints.h"
#include "cpu_internal.h"

#include <common/decaf_assert.h>
#include <common/platform_thread.h>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

struct
{
   std::atomic<bool> running { false };
   std::mutex mutex;
   std::condition_variable cv;
   std::thread thread;
} sAlarmData;

namespace cpu::internal
{

static void
alarmEntryPoint()
{
   while (sAlarmData.running) {
      std::unique_lock<std::mutex> lock{ sAlarmData.mutex };
      auto now = std::chrono::steady_clock::now();
      auto next = std::chrono::steady_clock::time_point::max();
      bool timedWait = false;

      for (auto i = 0; i < 3; ++i) {
         auto core = getCore(i);

         if (core->next_alarm <= now) {
            core->next_alarm = std::chrono::steady_clock::time_point::max();
            cpu::interrupt(i, ALARM_INTERRUPT);
         } else if (core->next_alarm < next) {
            next = core->next_alarm;
            timedWait = true;
         }
      }

      if (timedWait) {
         sAlarmData.cv.wait_until(lock, next);
      } else {
         sAlarmData.cv.wait(lock);
      }
   }
}

void
startAlarmThread()
{
   decaf_check(!sAlarmData.running.load());
   sAlarmData.running = true;
   sAlarmData.thread = std::thread { alarmEntryPoint };
   platform::setThreadName(&sAlarmData.thread, "CPU Alarm Thread");
}

void
joinAlarmThread()
{
   if (sAlarmData.thread.joinable()) {
      sAlarmData.thread.join();
   }
}

void
stopAlarmThread()
{
   sAlarmData.running = false;
   sAlarmData.cv.notify_all();
}

} // namespace cpu::internal

namespace cpu::this_core
{

void
setNextAlarm(std::chrono::steady_clock::time_point time)
{
   auto core = this_core::state();
   std::unique_lock<std::mutex> lock { sAlarmData.mutex };
   core->next_alarm = time;
   sAlarmData.cv.notify_all();
}

} // namespace cpu::this_core
