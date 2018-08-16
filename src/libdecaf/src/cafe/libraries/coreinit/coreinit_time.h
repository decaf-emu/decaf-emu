#pragma once
#include <chrono>
#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{

/**
 * \defgroup coreinit_time Time
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSCalendarTime
{
   // These are all from the POSIX tm struct, we are missing tm_isdst though.
   be2_val<int32_t> tm_sec;
   be2_val<int32_t> tm_min;
   be2_val<int32_t> tm_hour;
   be2_val<int32_t> tm_mday;
   be2_val<int32_t> tm_mon;
   be2_val<int32_t> tm_year;
   be2_val<int32_t> tm_wday;
   be2_val<int32_t> tm_yday;

   // Also Wii U has some extra fields not found in posix tm!
   be2_val<int32_t> tm_msec;
   be2_val<int32_t> tm_usec;
};
CHECK_OFFSET(OSCalendarTime, 0x00, tm_sec);
CHECK_OFFSET(OSCalendarTime, 0x04, tm_min);
CHECK_OFFSET(OSCalendarTime, 0x08, tm_hour);
CHECK_OFFSET(OSCalendarTime, 0x0C, tm_mday);
CHECK_OFFSET(OSCalendarTime, 0x10, tm_mon);
CHECK_OFFSET(OSCalendarTime, 0x14, tm_year);
CHECK_OFFSET(OSCalendarTime, 0x18, tm_wday);
CHECK_OFFSET(OSCalendarTime, 0x1C, tm_yday);
CHECK_OFFSET(OSCalendarTime, 0x20, tm_msec);
CHECK_OFFSET(OSCalendarTime, 0x24, tm_usec);
CHECK_SIZE(OSCalendarTime, 0x28);

#pragma pack(pop)

using OSTick = int32_t;

//! OSTime is ticks since epoch
using OSTime = int64_t;

using OSTimeSeconds = int64_t;
using OSTimeMilliseconds = int64_t;
using OSTimeMicroseconds = int64_t;
using OSTimeNanoseconds = int64_t;

OSTime
OSGetTime();

OSTime
OSGetSystemTime();

OSTick
OSGetTick();

OSTick
OSGetSystemTick();

void
OSTicksToCalendarTime(OSTime time,
                      virt_ptr<OSCalendarTime> calendarTime);

OSTime
OSCalendarTimeToTicks(virt_ptr<OSCalendarTime> calendarTime);

/** @} */

namespace internal
{

OSTime
msToTicks(OSTimeMilliseconds milliseconds);

OSTime
usToTicks(OSTimeMicroseconds microseconds);

OSTime
nsToTicks(OSTimeNanoseconds nanoseconds);

OSTimeMilliseconds
ticksToMs(OSTick ticks);

OSTime
getBaseTime();

std::chrono::time_point<std::chrono::system_clock>
toTimepoint(OSTime time);

OSTime
toOSTime(std::chrono::time_point<std::chrono::system_clock> chrono);

void
initialiseTime();

} // namespace internal

} // namespace cafe::coreinit
