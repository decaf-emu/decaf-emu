#include "filesystem_host_folder.h"
#include <common/platform.h>

#ifdef PLATFORM_POSIX
#include <sys/types.h>
#include <sys/stat.h>

namespace fs
{

Result<Folder *>
HostFolder::addFolder(const std::string &name)
{
   auto hostPath = mPath.join(name);
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

   if (mkdir(hostPath.path().c_str(), 0755)) {
      return Error::GenericError;
   }

   return reinterpret_cast<Folder *>(registerFolder(hostPath, name));
}


Result<Error>
HostFolder::remove(const std::string &name)
{
   auto hostPath = mPath.join(name);
   auto child = findChild(name);

   if (!child) {
      // File / Directory does not exist, nothing to do
      return Error::NotFound;
   }

   if (!checkPermission(Permissions::Write)) {
      return Error::InvalidPermission;
   }

   if (remove(hostPath.path().c_str())) {
      return Error::GenericError;
   }

   mVirtual.deleteChild(child);
   return Error::OK;
}


Node *
HostFolder::findChild(const std::string &name)
{
   struct stat data;
   auto hostPath = mPath.join(name);

   if (stat(hostPath.path().c_str(), &data)) {
      // File was not found
      if (auto child = mVirtual.findChild(name)) {
         // Delete the virtual child because file does not exist anymore
         mVirtual.deleteChild(child);
      }

      return nullptr;
   }

   // Setup node
   Node *child = nullptr;

   if (S_ISDIR(data.st_mode)) {
      child = registerFolder(hostPath, name);
   } else {
      child = registerFile(hostPath, name);
   }

   child->setSize(data.st_size);
   return child;
}


Result<Error>
HostFolder::hostMove(const HostPath &src,
                     const HostPath &dst)
{
   if (rename(src.path().c_str(), dst.path().c_str()) == 0) {
      return Error::OK;
   }

   return Error::GenericError;
}

} // namespace fs

#endif
