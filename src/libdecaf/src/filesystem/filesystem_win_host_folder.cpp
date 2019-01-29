#include "filesystem_host_folder.h"
#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#include <common/platform_winapi_string.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <direct.h>

namespace fs
{

Result<Folder *>
HostFolder::addFolder(const std::string &name)
{
   auto hostPath = mPath.join(name);
   auto winPath = platform::toWinApiString(hostPath.path());
   auto child = findChild(name);

   if (child) {
      if (child->type() != fs::Node::FolderNode) {
         return { Error::AlreadyExists, nullptr };
      }

      return { Error::AlreadyExists, reinterpret_cast<Folder *>(child) };
   }

   if (!checkPermission(Permissions::Write)) {
      return Error::InvalidPermission;
   }

   if (_wmkdir(winPath.c_str())) {
      return Error::GenericError;
   }

   return reinterpret_cast<Folder *>(registerFolder(hostPath, name));
}


Result<Error>
HostFolder::remove(const std::string &name)
{
   auto hostPath = mPath.join(name);
   auto winPath = platform::toWinApiString(hostPath.path());
   auto child = findChild(name);

   if (!child) {
      // File / Directory does not exist, nothing to do
      return Error::NotFound;
   }

   if (!checkPermission(Permissions::Write)) {
      return Error::InvalidPermission;
   }

   auto removed = false;

   if (child->type() == NodeType::FileNode) {
      removed = !!DeleteFileW(winPath.c_str());
   } else if (child->type() == NodeType::FolderNode) {
      removed = !!RemoveDirectoryW(winPath.c_str());
   }

   if (removed) {
      mVirtual.deleteChild(child);
   }

   return removed ? Error::OK : Error::GenericError;
}


Node *
HostFolder::findChild(const std::string &name)
{
   WIN32_FIND_DATAW data;
   auto hostPath = mPath.join(name);
   auto winPath = platform::toWinApiString(hostPath.path());

   // Find the file!
   auto handle = FindFirstFileW(winPath.c_str(), &data);

   if (handle == INVALID_HANDLE_VALUE) {
      // File was not found
      if (auto child = mVirtual.findChild(name)) {
         // Delete the virtual child because file does not exist anymore
         mVirtual.deleteChild(child);
      }

      return nullptr;
   }

   // Setup node
   Node *child = nullptr;

   if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      child = registerFolder(hostPath, name);
   } else {
      child = registerFile(hostPath, name);
   }

   auto size = size_t { data.nFileSizeLow };
   size |= (static_cast<size_t>(data.nFileSizeHigh) << 32);
   child->setSize(size);

   FindClose(handle);
   return child;
}


Result<Error>
HostFolder::hostMove(const HostPath &src,
                     const HostPath &dst)
{
   auto winSrcPath = platform::toWinApiString(src.path());
   auto winDstPath = platform::toWinApiString(dst.path());

   if (MoveFileW(winSrcPath.c_str(), winDstPath.c_str())) {
      return Error::OK;
   }

   return Error::GenericError;
}

} // namespace fs

#endif
