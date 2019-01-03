#include "coreinit.h"
#include "coreinit_time.h"
#include "coreinit_systeminfo.h"

#include "decaf_config.h"
#include "decaf_configstorage.h"

#include <common/platform_time.h>
#include <libcpu/cpu.h>
#include <thread>

namespace cafe::coreinit
{

struct TimeData
{
   std::chrono::time_point<std::chrono::system_clock> epochTime;
   std::chrono::time_point<std::chrono::system_clock> baseClock;
   cpu::TimerDuration baseTicks;
};

static virt_ptr<TimeData> sTimeData = nullptr;
static std::atomic<bool> sTimeScaleEnabled = false;
static std::atomic<double> sTimeScale = 1.0;

static uint64_t
scaledTimebase()
{
   auto timeBase = cpu::this_core::state()->tb();
   if (!sTimeScaleEnabled.load(std::memory_order_relaxed)) {
      return timeBase;
   }

   auto timeScale = sTimeScale.load(std::memory_order_relaxed);
   return static_cast<uint64_t>(static_cast<double>(timeBase) * timeScale);
}

/**
 * Time since epoch
 */
OSTime
OSGetTime()
{
   return OSGetSystemTime() + sTimeData->baseTicks.count();
}


/**
 * Time since system start up
 */
OSTime
OSGetSystemTime()
{
   return scaledTimebase();
}


/**
 * Ticks since epoch
 */
OSTick
OSGetTick()
{
   return OSGetSystemTick() + static_cast<OSTick>(sTimeData->baseTicks.count());
}


/**
 * Ticks since system start up
 */
OSTick
OSGetSystemTick()
{
   return static_cast<uint32_t>(scaledTimebase());
}


/**
 * Convert OSTime to OSCalendarTime
 */
void
OSTicksToCalendarTime(OSTime time,
                      virt_ptr<OSCalendarTime> calendarTime)
{
   auto chrono = coreinit::internal::toTimepoint(time);
   std::time_t system_time_t = std::chrono::system_clock::to_time_t(chrono);
   std::tm tm = platform::localtime(system_time_t);

   calendarTime->tm_sec = tm.tm_sec;
   calendarTime->tm_min = tm.tm_min;
   calendarTime->tm_hour = tm.tm_hour;
   calendarTime->tm_mday = tm.tm_mday;
   calendarTime->tm_mon = tm.tm_mon;
   calendarTime->tm_year = tm.tm_year + 1900; // posix tm_year is year - 1900
   calendarTime->tm_wday = tm.tm_wday;
   calendarTime->tm_yday = tm.tm_yday;
   calendarTime->tm_msec = 0;
   calendarTime->tm_usec = 0;

   // TODO: OSTicksToCalendarTime tm_usec, tm_msec
}


/**
 * Convert OSCalendarTime to OSTime
 */
OSTime
OSCalendarTimeToTicks(virt_ptr<OSCalendarTime> calendarTime)
{
   std::tm tm = { 0 };

   tm.tm_sec = calendarTime->tm_sec;
   tm.tm_min = calendarTime->tm_min;
   tm.tm_hour = calendarTime->tm_hour;
   tm.tm_mday = calendarTime->tm_mday;
   tm.tm_mon = calendarTime->tm_mon;
   tm.tm_year = calendarTime->tm_year - 1900;
   tm.tm_wday = calendarTime->tm_wday;
   tm.tm_yday = calendarTime->tm_yday;
   tm.tm_isdst = -1;

   auto system_time = platform::make_gm_time(tm);
   auto chrono = std::chrono::system_clock::from_time_t(system_time);

   // Add on tm_usec, tm_msec which is missing from std::tm
   chrono += std::chrono::microseconds { calendarTime->tm_usec };
   chrono += std::chrono::milliseconds { calendarTime->tm_msec };

   return coreinit::internal::toOSTime(chrono);
}

namespace internal
{

OSTime
msToTicks(OSTimeMilliseconds milliseconds)
{
   auto timerSpeed = static_cast<uint64_t>(OSGetSystemInfo()->busSpeed / 4);
   return static_cast<uint64_t>(milliseconds) * (timerSpeed / 1000);
}

OSTime
usToTicks(OSTimeMicroseconds microseconds)
{
   auto timerSpeed = static_cast<uint64_t>(OSGetSystemInfo()->busSpeed / 4);
   return static_cast<uint64_t>(microseconds) * (timerSpeed / 1000000);
}

OSTime
nsToTicks(OSTimeNanoseconds nanoseconds)
{
   // Division is done in two parts to try to maintain accuracy, 31250 * 32000 = 1*10^9
   auto timerSpeed = static_cast<uint64_t>(OSGetSystemInfo()->busSpeed / 4);
   return (static_cast<uint64_t>(nanoseconds) * (timerSpeed / 31250)) / 32000;
}

OSTimeMilliseconds
ticksToMs(OSTick ticks)
{
   auto timerSpeed = static_cast<uint64_t>(OSGetSystemInfo()->busSpeed / 4);
   return (static_cast<OSTimeMilliseconds>(ticks) * 1000) / timerSpeed;
}

OSTime
getBaseTime()
{
   return sTimeData->baseTicks.count();
}

std::chrono::time_point<std::chrono::system_clock>
toTimepoint(OSTime time)
{
   auto ticksSinceBaseTime = cpu::TimerDuration { time } - sTimeData->baseTicks;
   auto clocksSinceBaseTime = std::chrono::duration_cast<std::chrono::system_clock::duration>(ticksSinceBaseTime);
   return sTimeData->baseClock + clocksSinceBaseTime;
}

OSTime
toOSTime(std::chrono::time_point<std::chrono::system_clock> chrono)
{
   auto clocksSinceBaseTime = chrono - sTimeData->baseClock;
   auto ticksSinceBaseTime = std::chrono::duration_cast<cpu::TimerDuration>(clocksSinceBaseTime);
   return (sTimeData->baseTicks + ticksSinceBaseTime).count();
}

void
initialiseTime()
{
   static std::once_flag sRegisteredConfigChangeListener;
   std::call_once(sRegisteredConfigChangeListener,
      []() {
         decaf::registerConfigChangeListener(
            [](const decaf::Settings &settings) {
               sTimeScale = settings.system.time_scale;
               sTimeScaleEnabled = settings.system.time_scale_enabled;
            });
      });
   sTimeScale = decaf::config()->system.time_scale;
   sTimeScaleEnabled = decaf::config()->system.time_scale_enabled;

   // Calculate the Wii U epoch (01/01/2000)
   std::tm tm = { 0 };
   tm.tm_sec = 0;
   tm.tm_min = 0;
   tm.tm_hour = 0;
   tm.tm_mday = 1;
   tm.tm_mon = 1;
   tm.tm_year = 2000 - 1900;
   tm.tm_isdst = -1;
   sTimeData->epochTime = std::chrono::system_clock::from_time_t(platform::make_gm_time(tm));

   sTimeData->baseClock = std::chrono::system_clock::now();
   auto ticksSinceEpoch = std::chrono::duration_cast<cpu::TimerDuration>(sTimeData->baseClock - sTimeData->epochTime);
   auto ticksSinceStart = cpu::TimerDuration { cpu::this_core::state()->tb() };
   sTimeData->baseTicks = ticksSinceEpoch - ticksSinceStart;
}

} // namespace internal

void
Library::registerTimeSymbols()
{
   RegisterFunctionExport(OSGetTime);
   RegisterFunctionExport(OSGetTick);
   RegisterFunctionExport(OSGetSystemTime);
   RegisterFunctionExport(OSGetSystemTick);
   RegisterFunctionExport(OSTicksToCalendarTime);
   RegisterFunctionExport(OSCalendarTimeToTicks);

   RegisterDataInternal(sTimeData);
}

} // namespace cafe::coreinit
