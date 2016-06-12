#pragma once
#include <thread>
#include <string>

namespace platform
{

void
setThreadName(std::thread *thread,
			  const std::string &name);

void
exitThread(int result);

} // namespace platform
