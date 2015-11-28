#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#include <io.h>

namespace platform
{

bool
createDirectory(const std::string &path)
{
   return _mkdir(path.c_str()) == 0;
}

bool
fileExists(const std::string &path)
{
   return _access_s(path.c_str(), 0) == 0;
}

} // namespace platform

#endif
