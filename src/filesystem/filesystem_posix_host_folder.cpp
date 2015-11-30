#include "filesystem_host_folder.h"
#include "platform/platform.h"

#ifdef PLATFORM_POSIX
#include <sys/types.h>
#include <sys/stat.h>

namespace fs
{

Node *
HostFolder::findChild(const std::string &name)
{
   struct stat data;
   auto child = mVirtual.findChild(name);

   if (child) {
      return child;
   }

   auto hostPath = mPath.join(name);

   if (stat(hostPath.path().c_str(), &data)) {
      return nullptr;
   }

   if (S_ISDIR(data.st_mode)) {
      child = addChild(new HostFolder(hostPath, name));
   } else {
      child = addChild(new HostFile(hostPath, name));
   }

   child->size = data.st_size;
   return child;
}

} // namespace fs

#endif
