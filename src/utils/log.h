#pragma once
#include <spdlog/spdlog.h>

extern std::shared_ptr<spdlog::logger>
gLog;

namespace logging
{

void
initialise(const std::string &filename);

} // namespace logging
