#pragma once
#include <string>

namespace platform
{

bool
createDirectory(const std::string &path);

bool
createParentDirectories(const std::string &path);

bool
fileExists(const std::string &path);

bool
isFile(const std::string &path);

bool
isDirectory(const std::string &path);

std::string
getConfigDirectory();

} // namespace platform
