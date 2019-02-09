#include "ios_alarm_thread.h"
#include "ios/kernel/ios_kernel_hardware.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>

namespace ios::internal
{

static std::thread
sAlarmThread;

static std::mutex
sAlarmMutex;

static std::condition_variable
sAlarmCondition;

static std::atomic_bool
sAlarmThreadRunning = false;

static std::chrono::steady_clock::time_point
sNextAlarm = std::chrono::steady_clock::time_point::max();

static void
alarmThread()
{
   while (sAlarmThreadRunning.load()) {
      std::unique_lock<std::mutex> lock { sAlarmMutex };
      auto now = std::chrono::steady_clock::now();

      if (now >= sNextAlarm) {
         sNextAlarm = std::chrono::steady_clock::time_point::max();
         lock.unlock();
         kernel::internal::setInterruptAhbAll(kernel::AHBALL::get(0).Timer(true));
         lock.lock();
      }

      if (sNextAlarm != std::chrono::steady_clock::time_point::max()) {
         sAlarmCondition.wait_until(lock, sNextAlarm);
      } else {
         sAlarmCondition.wait(lock);
      }
   }
}

void
startAlarmThread()
{
   if(!sAlarmThreadRunning.load()) {
      sAlarmThreadRunning.store(true);
      sAlarmThread = std::thread { alarmThread };
   }
}

void
stopAlarmThread()
{
   if(sAlarmThreadRunning.load()) {
      sAlarmThreadRunning.store(false);
      sAlarmCondition.notify_all();
      sAlarmThread.join();
   }
}

void
setNextAlarm(std::chrono::steady_clock::time_point time)
{
   std::unique_lock<std::mutex> lock { sAlarmMutex };
   sNextAlarm = time;
   sAlarmCondition.notify_all();
}

} // namespace ios::internal
