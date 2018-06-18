#pragma once
#include <chrono>

namespace ios::internal
{

void
startAlarmThread();

void
joinAlarmThread();

void
setNextAlarm(std::chrono::steady_clock::time_point time);

} // namespace ios::internal
