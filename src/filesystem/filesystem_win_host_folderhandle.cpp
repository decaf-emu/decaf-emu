#include "filesystem_host_folderhandle.h"
#include "platform/platform.h"

#ifdef PLATFORM_WINDOWS
#include <Windows.h>

namespace fs
{

struct WindowsData
{
   WIN32_FIND_DATAA data;
   HANDLE handle = INVALID_HANDLE_VALUE;
};

bool HostFolderHandle::open()
{
   auto data = reinterpret_cast<WindowsData *>(mFindData);

   if (!data) {
      data = new WindowsData();
   }

   data->handle = FindFirstFileA(mPath.join("*").path().c_str(), &data->data);

   if (data->handle == INVALID_HANDLE_VALUE) {
      delete data;
      mFindData = nullptr;
      return false;
   }

   if (mVirtualHandle) {
      mVirtualHandle->open();
   }

   return true;
}

void HostFolderHandle::close()
{
   auto data = reinterpret_cast<WindowsData *>(mFindData);

   if (data) {
      FindClose(data->handle);
      delete data;
      mFindData = nullptr;
   }

   if (mVirtualHandle) {
      mVirtualHandle->close();
   }
}

bool HostFolderHandle::read(FolderEntry &entry)
{
   if (mVirtualHandle->read(entry)) {
      return true;
   }

   if (!mFindData) {
      return false;
   }

   auto data = reinterpret_cast<WindowsData *>(mFindData);

   entry.name = data->data.cFileName;
   entry.size = data->data.nFileSizeLow;
   entry.size |= (static_cast<size_t>(data->data.nFileSizeHigh) << 32);

   if (data->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      entry.type = FolderEntry::Folder;
   } else {
      entry.type = FolderEntry::File;
   }

   if (!FindNextFileA(data->handle, &data->data)) {
      close();
   }

   if (entry.name.compare(".") == 0 || entry.name.compare("..") == 0) {
      // Skip over . and .. entries
      return read(entry);
   }

   return true;
}

bool HostFolderHandle::rewind()
{
   close();
   return open();
}

} // namespace fs

#endif
