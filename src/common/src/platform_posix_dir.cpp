#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_POSIX
#include <sys/stat.h>
#include <unistd.h>

namespace platform
{

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

bool
isFile(const std::string &path)
{
   struct stat info;
   auto result = stat(path.c_str(), &info);

   if (result != 0) {
      return false;
   }

   return S_ISREG(info.st_mode);
}

bool
isDirectory(const std::string &path)
{
   struct stat info;
   auto result = stat(path.c_str(), &info);

   if (result != 0) {
      return false;
   }

   return S_ISDIR(info.st_mode);
}

} // namespace platform

#endif
