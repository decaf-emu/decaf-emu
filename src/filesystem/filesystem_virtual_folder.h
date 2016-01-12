#pragma once
#include <vector>
#include "filesystem_folder.h"
#include "filesystem_node.h"
#include "filesystem_virtual_file.h"
#include "filesystem_virtual_folderhandle.h"

namespace fs
{

class VirtualFolder : public Folder
{
public:
   VirtualFolder(std::string name) :
      Folder(name)
   {
   }

   virtual ~VirtualFolder() override
   {
      for (auto node : mChildren) {
         delete node;
      }
   }

   virtual Node *addFolder(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node) {
         node = new VirtualFolder(name);
         addChild(node);
      }

      return node;
   }

   virtual Node *addFile(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node) {
         node = new VirtualFile(name);
         addChild(node);
      }

      return node;
   }

   virtual Node *addChild(Node *node) override
   {
      mChildren.push_back(node);
      return node;
   }

   virtual bool deleteFile(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node || node->type != Node::FileNode) {
         return false;
      }

      mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), node), mChildren.end());
      delete node;
      return true;
   }

   virtual bool deleteFolder(const std::string &name) override
   {
      auto node = findChild(name);

      if (!node || node->type != Node::FolderNode) {
         return false;
      }

      mChildren.erase(std::remove(mChildren.begin(), mChildren.end(), node), mChildren.end());
      delete node;
      return true;
   }

   virtual bool deleteChild(Node *node) override
   {
      auto itr = std::remove(mChildren.begin(), mChildren.end(), node);

      if (itr == mChildren.end()) {
         return false;
      }

      mChildren.erase(itr, mChildren.end());
      delete node;
      return true;
   }

   virtual Node *findChild(const std::string &name) override
   {
      for (auto node : mChildren) {
         if (node->name.compare(name) == 0) {
            return node;
         }
      }

      return nullptr;
   }

   virtual FolderHandle *open() override
   {
      auto handle = new VirtualFolderHandle(mChildren.begin(), mChildren.end());

      if (!handle->open()) {
         delete handle;
         return nullptr;
      }

      return handle;
   }

private:
   std::vector<Node *> mChildren;
};

} // namespace fs
