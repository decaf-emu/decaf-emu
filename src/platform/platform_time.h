#pragma once
#include <ctime>

namespace platform
{

tm
localtime(const std::time_t& time);

time_t
make_gm_time(std::tm time);

} // namespace platform
