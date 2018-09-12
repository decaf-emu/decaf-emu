#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_POSIX
#include <errno.h>
#include <fmt/format.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

namespace platform
{

bool
createDirectory(const std::string &path)
{
   if (!createParentDirectories(path)) {
      return false;
   }

   return mkdir(path.c_str(), 0755) == 0
      || (errno == EEXIST && isDirectory(path));
}

bool
createParentDirectories(const std::string &path)
{
   auto slashPos = path.rfind('/');

   if (slashPos == std::string::npos || slashPos == 0) {
      return true;
   }

   return createDirectory(path.substr(0, slashPos));
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

std::string
getConfigDirectory()
{
   // TODO: will be different for Mac
   const char *home = getenv("HOME");
   if (home && *home) {
      return fmt::format("{}/.config", home);
   } else {
      return ".";
   }
}

} // namespace platform

#endif
