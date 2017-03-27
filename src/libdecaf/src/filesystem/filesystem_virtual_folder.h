#pragma once
#include "filesystem_folder.h"
#include "filesystem_node.h"
#include "filesystem_virtual_folderhandle.h"
#include <algorithm>
#include <vector>

namespace fs
{

class VirtualFolder : public Folder
{
public:
   VirtualFolder(Permissions permissions,
                 const std::string &name) :
      Folder(DeviceType::VirtualDevice, permissions, name)
   {
   }

   virtual ~VirtualFolder() override
   {
      for (auto node : mChildren) {
         delete node;
      }
   }

   Node *
   addChild(Node *node)
   {
      mChildren.push_back(node);
      return node;
   }

   bool
   deleteChild(Node *node)
   {
      auto itr = std::remove(mChildren.begin(), mChildren.end(), node);

      if (itr == mChildren.end()) {
         return false;
      }

      mChildren.erase(itr, mChildren.end());
      delete node;
      return true;
   }

   virtual Result<Folder *>
   addFolder(const std::string &name) override
   {
      if (!checkPermission(Permissions::Write)) {
         return Error::InvalidPermission;
      }

      auto node = findChild(name);

      if (node) {
         if (node->type() != fs::Node::FolderNode) {
            return { Error::AlreadyExists, nullptr };
         }

         return { Error::AlreadyExists, reinterpret_cast<Folder *>(node) };
      }

      node = new VirtualFolder { mPermissions, name };
      addChild(node);
      return reinterpret_cast<Folder *>(node);
   }

   virtual Result<FileHandle>
   openFile(const std::string &name,
            File::OpenMode mode) override
   {
      return { Error::UnsupportedOperation };
   }

   virtual Result<Error>
   remove(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node) {
         return Error::NotFound;
      }

      mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), node), mChildren.end());
      delete node;
      return Error::OK;
   }

   virtual Node *
   findChild(const std::string &name) override
   {
      for (auto node : mChildren) {
         if (node->name().compare(name) == 0) {
            return node;
         }
      }

      return nullptr;
   }

   virtual Result<FolderHandle>
   openDirectory() override
   {
      if (!checkPermission(Permissions::Read)) {
         return Error::InvalidPermission;
      }

      auto handle = new VirtualFolderHandle { mChildren.begin(), mChildren.end() };

      if (!handle->open()) {
         delete handle;
         return Error::UnsupportedOperation;
      }

      return FolderHandle { handle };
   }

   virtual void
   setPermissions(Permissions permissions,
                  PermissionFlags flags) override
   {
      mPermissions = permissions;

      if (flags & PermissionFlags::Recursive) {
         for (auto node : mChildren) {
            node->setPermissions(permissions, flags);
         }
      }
   }

private:
   std::vector<Node *> mChildren;
};

} // namespace fs
