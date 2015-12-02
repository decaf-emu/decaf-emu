#pragma once
#include <vector>
#include "filesystem_folder.h"
#include "filesystem_node.h"
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

   virtual Node *addChild(Node *node) override
   {
      mChildren.push_back(node);
      return node;
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
