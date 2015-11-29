#include "filesystem_host_folder.h"
#include "platform/platform.h"

#ifdef PLATFORM_POSIX

namespace fs
{

Node *
HostFolder::findChild(const std::string &name)
{
   return nullptr;
}

} // namespace fs

#endif
