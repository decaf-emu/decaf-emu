#pragma once
#include <thread>
#include <string>
#include "platform/platform.h"

namespace platform
{

void
setThreadName(std::thread *thread, const std::string& name);

} // namespace platform
