#pragma once
#include "filesystem_folder.h"
#include "filesystem_host_file.h"
#include "filesystem_host_folderhandle.h"
#include "filesystem_host_path.h"
#include "filesystem_virtual_folder.h"

namespace fs
{

class HostFolder : public Folder
{
public:
   HostFolder(const HostPath &path, const std::string &name, Permissions permissions) :
      Folder(DeviceType::HostDevice, permissions, name),
      mPath(path),
      mVirtual(permissions, name)
   {
   }

   virtual ~HostFolder() override = default;

   virtual Result<Folder *>
   addFolder(const std::string &name) override;

   virtual Result<Error>
   remove(const std::string &name) override;

   virtual Node *
   findChild(const std::string &name) override;

   virtual Result<FileHandle>
   openFile(const std::string &name,
            File::OpenMode mode) override
   {
      auto hostPath = mPath.join(name);
      auto child = findChild(name);

      if ((mode & File::Write) || (mode & File::Append)) {
         // Check we have write permission
         if (!checkPermission(Permissions::Write)) {
            return Error::InvalidPermission;
         }

         // In Write/Append mode create file if not found.
         if (!child) {
            child = registerFile(hostPath, name);
         }
      }

      if (!child) {
         return Error::NotFound;
      }

      // Ensure we have read permissions
      if (!checkPermission(Permissions::Read)) {
         return Error::InvalidPermission;
      }

      // Ensure this is a file node
      if (child->type() != NodeType::FileNode) {
         return Error::NotFile;
      }

      return reinterpret_cast<File *>(child)->open(mode);
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags) override
   {
      mPermissions = permissions;
      mVirtual.setPermissions(permissions, flags);
   }

   virtual Result<FolderHandle>
   openDirectory() override
   {
      if (!checkPermission(Permissions::Read)) {
         return Error::InvalidPermission;
      }

      auto handle = new HostFolderHandle { mPath };

      if (!handle->open()) {
         delete handle;
         return Error::UnsupportedOperation;
      }

      return FolderHandle { handle };
   }

   static Result<Error>
   move(HostFolder *srcFolder,
        const std::string &srcName,
        HostFolder *dstFolder,
        const std::string &dstName)
   {
      // Check our permissions
      if (!srcFolder->checkPermission(fs::Permissions::ReadWrite)) {
         return Error::InvalidPermission;
      }

      if (!dstFolder->checkPermission(fs::Permissions::Write)) {
         return Error::InvalidPermission;
      }

      // Ensure src exists
      auto srcChild = srcFolder->findChild(srcName);

      if (!srcChild) {
         return Error::NotFound;
      }

      // Ensure dst does not exist
      auto dstChild = dstFolder->findChild(dstName);

      if (dstChild) {
         return Error::AlreadyExists;
      }

      // Get host paths
      auto srcHostPath = srcFolder->mPath.join(srcName);
      auto dstHostPath = dstFolder->mPath.join(dstName);

      // Perform the move operation on the host device
      auto result = hostMove(srcHostPath, dstHostPath);

      if (!result) {
         return result;
      }

      // Unregister the child from srcFolder as it no longer exists once moved
      srcFolder->mVirtual.deleteChild(srcChild);
      return Error::OK;
   }

private:
   Node *
   registerFile(const HostPath &path,
                const std::string &name)
   {
      auto child = mVirtual.findChild(name);

      if (child && child->type() != NodeType::FileNode) {
         mVirtual.deleteChild(child);
         child = nullptr;
      }

      if (!child) {
         child = new HostFile { path, name, mPermissions };
         mVirtual.addChild(child);
      }

      return child;
   }

   Node *
   registerFolder(const HostPath &path,
                  const std::string &name)
   {
      auto child = mVirtual.findChild(name);

      if (child && child->type() != NodeType::FolderNode) {
         mVirtual.deleteChild(child);
         child = nullptr;
      }

      if (!child) {
         child = new HostFolder { path, name, mPermissions };
         mVirtual.addChild(child);
      }

      return child;
   }

   static Result<Error>
   hostMove(const HostPath &src,
            const HostPath &dst);

private:
   HostPath mPath;
   VirtualFolder mVirtual;
};

} // namespace fs
