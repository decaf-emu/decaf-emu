#include "filesystem_host_folderhandle.h"
#include "platform/platform.h"

#ifdef PLATFORM_POSIX

namespace fs
{

bool HostFolderHandle::open()
{
   return false;
}

void HostFolderHandle::close()
{
}

bool HostFolderHandle::read(FolderEntry &entry)
{
   return false;
}

bool HostFolderHandle::rewind()
{
   return false;
}

} // namespace fs

#endif
