#pragma once
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <vector>

namespace decaf
{

void
initialiseLogging(std::string_view filename);

std::shared_ptr<spdlog::logger>
makeLogger(std::string name,
           std::vector<spdlog::sink_ptr> userSinks = {});

void
setLogLevel(spdlog::level::level_enum level);

} // namespace decaf
