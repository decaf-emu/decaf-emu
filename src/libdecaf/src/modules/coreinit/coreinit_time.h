#pragma once
#include <chrono>
#include "common/be_val.h"
#include "common/structsize.h"

namespace coreinit
{

/**
 * \defgroup coreinit_time Time
 * \ingroup coreinit
 * @{
 */

#pragma pack(push, 1)

struct OSCalendarTime
{
   // fields mostly match Posix's struct tm, so names are taken from that struct
   be_val<int32_t> tm_sec;
   be_val<int32_t> tm_min;
   be_val<int32_t> tm_hour;
   be_val<int32_t> tm_mday;
   be_val<int32_t> tm_mon;
   be_val<int32_t> tm_year;
};
CHECK_OFFSET(OSCalendarTime, 0x00, tm_sec);
CHECK_OFFSET(OSCalendarTime, 0x04, tm_min);
CHECK_OFFSET(OSCalendarTime, 0x08, tm_hour);
CHECK_OFFSET(OSCalendarTime, 0x0C, tm_mday);
CHECK_OFFSET(OSCalendarTime, 0x10, tm_mon);
CHECK_OFFSET(OSCalendarTime, 0x14, tm_year);
CHECK_SIZE(OSCalendarTime, 0x18);

#pragma pack(pop)

//! OSTick is 1 nanosecond
using OSTick = int32_t;

//! OSTime is ticks since epoch
using OSTime = int64_t;

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
                      OSCalendarTime *calendarTime);

OSTime
OSCalendarTimeToTicks(OSCalendarTime *calendarTime);

/** @} */

namespace internal
{

OSTime
getBaseTime();

std::chrono::time_point<std::chrono::system_clock>
toTimepoint(OSTime time);

OSTime
toOSTime(std::chrono::time_point<std::chrono::system_clock> chrono);

} // namespace internal

} // namespace coreinit
