#include "filesystem_host_folderhandle.h"
#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#include <common/platform_winapi_string.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace fs
{

struct WindowsData
{
   WIN32_FIND_DATAW data;
   HANDLE handle = INVALID_HANDLE_VALUE;
};


bool
HostFolderHandle::open()
{
   auto data = reinterpret_cast<WindowsData *>(mFindData);
   auto hostPath = mPath.join("*");
   auto winPath = platform::toWinApiString(hostPath.path());

   if (!data) {
      data = new WindowsData();
      mFindData = data;
   }

   data->handle = FindFirstFileW(winPath.c_str(), &data->data);

   if (data->handle == INVALID_HANDLE_VALUE) {
      delete data;
      mFindData = nullptr;
      return false;
   }

   return true;
}


void
HostFolderHandle::close()
{
   auto data = reinterpret_cast<WindowsData *>(mFindData);

   if (data) {
      FindClose(data->handle);
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

   auto data = reinterpret_cast<WindowsData *>(mFindData);
   entry.name = platform::fromWinApiString(data->data.cFileName);
   entry.size = data->data.nFileSizeLow;
   entry.size |= (static_cast<size_t>(data->data.nFileSizeHigh) << 32);

   if (data->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      entry.type = FolderEntry::Folder;
   } else {
      entry.type = FolderEntry::File;
   }

   if (!FindNextFileW(data->handle, &data->data)) {
      close();
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
   close();
   return open();
}

} // namespace fs

#endif
