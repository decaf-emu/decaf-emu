#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_WINDOWS
#include <direct.h>
#include <io.h>
#include <sys/stat.h>

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

bool
isFile(const std::string &path)
{
   struct _stat info;
   auto result = _stat(path.c_str(), &info);

   if (result != 0) {
      return false;
   }

   return !!(info.st_mode & _S_IFREG);
}

bool
isDirectory(const std::string &path)
{
   struct _stat info;
   auto result = _stat(path.c_str(), &info);

   if (result != 0) {
      return false;
   }

   return !!(info.st_mode & _S_IFDIR);
}

} // namespace platform

#endif
