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

   virtual Node *
   addFolder(const std::string &name) override;

   virtual bool
   remove(const std::string &name) override;

   virtual Node *
   findChild(const std::string &name) override;

   virtual FileHandle *
   openFile(const std::string &name,
            File::OpenMode mode) override
   {
      auto hostPath = mPath.join(name);
      auto child = findChild(name);

      if (!child && checkPermission(Permissions::Write)) {
         if ((mode & File::Write) || (mode & File::Append)) {
            // Create file in Write or Append mode
            child = registerFile(hostPath, name);
         }
      }

      if (!child) {
         return nullptr;
      }

      // Ensure we have read permissions
      if (!checkPermission(Permissions::Read)) {
         return nullptr;
      }

      // Ensure this is a file node
      if (child->type() != NodeType::FileNode) {
         return nullptr;
      }

      return reinterpret_cast<File *>(child)->open(mode);
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags)
   {
      mPermissions = permissions;
      mVirtual.setPermissions(permissions, flags);
   }

   virtual FolderHandle *
   openDirectory() override
   {
      if (!checkPermission(Permissions::Read)) {
         return nullptr;
      }

      auto handle = new HostFolderHandle { mPath };

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return handle;
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

private:
   HostPath mPath;
   VirtualFolder mVirtual;
};

} // namespace fs
