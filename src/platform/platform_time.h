#pragma once
#include <ctime>
#include "platform/platform.h"

namespace platform
{

tm localtime(const std::time_t& time);

} // namespace platform
