#pragma once
#include <string>

namespace platform
{

bool
createDirectory(const std::string &path);

bool
fileExists(const std::string &path);

} // namespace platform
