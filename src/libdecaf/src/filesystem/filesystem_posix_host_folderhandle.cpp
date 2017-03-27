#include "filesystem_host_folderhandle.h"
#include <common/platform.h>

#ifdef PLATFORM_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

namespace fs
{

struct PosixData
{
   DIR *handle;
};


bool
HostFolderHandle::open()
{
   auto data = reinterpret_cast<PosixData *>(mFindData);

   if (!data) {
      data = new PosixData();
      mFindData = data;
   }

   data->handle = opendir(mPath.path().c_str());

   if (!data->handle) {
      delete data;
      mFindData = nullptr;
      return false;
   }

   return true;
}


void
HostFolderHandle::close()
{
   auto data = reinterpret_cast<PosixData *>(mFindData);

   if (data) {
      closedir(data->handle);
      delete data;
      mFindData = nullptr;
   }
}


bool
HostFolderHandle::read(FolderEntry &entry)
{
   if (!mFindData) {
      return false;
   }

   auto data = reinterpret_cast<PosixData *>(mFindData);
   auto item = readdir(data->handle);

   if (!item) {
      return false;
   }

   entry.name = item->d_name;
   entry.size = 0;

   if (item->d_type == DT_DIR) {
      entry.type = FolderEntry::Folder;
   } else {
      struct stat st;

      if (stat(mPath.join(entry.name).path().c_str(), &st) == 0) {
         entry.size = st.st_size;
      }

      entry.type = FolderEntry::File;
   }

   if (entry.name.compare(".") == 0 || entry.name.compare("..") == 0) {
      // Skip over . and .. entries
      return read(entry);
   }

   return true;
}


bool
HostFolderHandle::rewind()
{
   auto data = reinterpret_cast<PosixData *>(mFindData);

   if (!data) {
      return false;
   }

   rewinddir(data->handle);
   return true;
}

} // namespace fs

#endif
