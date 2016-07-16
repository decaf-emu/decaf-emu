#include "platform.h"
#include "platform_dir.h"

#ifdef PLATFORM_WINDOWS
#include <codecvt>
#include <direct.h>
#include <errno.h>
#include <io.h>
#include <locale>
#include <sys/stat.h>
#include <ShlObj.h>

namespace platform
{

static bool
isDriveName(const std::string &path)
{
   return path.length() == 2
      && ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z'))
      && path[1] == ':';
}

bool
createDirectory(const std::string &path)
{
   if (!createParentDirectories(path)) {
      return false;
   }

   return _mkdir(path.c_str()) == 0
      || (errno == EEXIST && isDirectory(path));
}

bool
createParentDirectories(const std::string &path)
{
   auto slashPos = path.find_last_of("/\\");
   if (slashPos == std::string::npos
       || (slashPos == 2 && isDriveName(path.substr(0, 2)))
       || (path.find_first_not_of("/\\") == 2  // "\\server\path" syntax
           && path.find_first_of("/\\", 2) == slashPos)) {
      return true;
   }

   return createDirectory(path.substr(0, slashPos));
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

std::string
getConfigDirectory()
{
   PWSTR path;
   auto result = std::string { "." };

   if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path))) {
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      result = converter.to_bytes(path);
      CoTaskMemFree(path);
   }

   return result;
}

} // namespace platform

#endif
