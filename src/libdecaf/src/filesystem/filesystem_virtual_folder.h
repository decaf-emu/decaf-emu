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

   virtual Node *
   addFolder(const std::string &name) override
   {
      if (!checkPermission(Permissions::Write)) {
         return nullptr;
      }

      auto node = findChild(name);

      if (!node) {
         node = new VirtualFolder { mPermissions, name };
         addChild(node);
      }

      return node;
   }

   virtual FileHandle
   openFile(const std::string &name,
            File::OpenMode mode) override
   {
      return nullptr;
   }

   virtual bool
   remove(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node) {
         return false;
      }

      mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), node), mChildren.end());
      delete node;
      return true;
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

   virtual FolderHandle
   openDirectory() override
   {
      if (!checkPermission(Permissions::Read)) {
         return nullptr;
      }

      auto handle = new VirtualFolderHandle { mChildren.begin(), mChildren.end() };

      if (!handle->open()) {
         delete handle;
         return nullptr;
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
