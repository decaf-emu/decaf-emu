#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_POSIX
#include <sys/stat.h>
#include <unistd.h>

bool
createDirectory(const std::string &path)
{
   return mkdir(path.c_str(), 0755) == 0;
}

bool
fileExists(const std::string &path)
{
   return access(path.c_str(), F_OK) != -1;
}

} // namespace platform

#endif
