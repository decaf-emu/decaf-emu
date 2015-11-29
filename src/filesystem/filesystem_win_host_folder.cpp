#include "filesystem_host_folder.h"
#include "platform/platform.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace fs
{

Node *
HostFolder::findChild(const std::string &name)
{
   auto child = mVirtual.findChild(name);

   if (child) {
      return child;
   }

   WIN32_FIND_DATAA data;
   auto hostPath = mPath.join(name);
   auto handle = FindFirstFileA(hostPath.path().c_str(), &data);

   if (handle == INVALID_HANDLE_VALUE) {
      // File not found!
      return nullptr;
   }

   // File/Directory found, create matching virtual entry
   if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      child = addChild(new HostFolder(hostPath, data.cFileName));
   } else {
      child = addChild(new HostFile(hostPath, data.cFileName));
   }

   child->size = data.nFileSizeLow;
   child->size |= (static_cast<size_t>(data.nFileSizeHigh) << 32);

   FindClose(handle);
   return child;
}

} // namespace fs

#endif
