#pragma once
#include <chrono>

namespace gpu::clock
{

using Time = int64_t;

inline Time
now()
{
   return std::chrono::steady_clock::now().time_since_epoch().count();
}

} // namespace gpu::clock
