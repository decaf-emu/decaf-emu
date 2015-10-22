#pragma once
#include <thread>
#include <string>
#include "platform/platform.h"

namespace platform
{

void
set_thread_name(std::thread *thread, const std::string& name);

} // namespace platform
