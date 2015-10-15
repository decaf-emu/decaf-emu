#pragma once
#include "filesystem_folder.h"

namespace fs
{

struct VirtualFolder : Folder
{
   VirtualFolder(std::string name) :
      Folder(name, FileSystemNode::VirtualNode)
   {
   }

   virtual ~VirtualFolder()
   {
      for (auto node : children) {
         delete node;
      }
   }

   virtual bool open()
   {
      pos = children.begin();
      return true;
   }

   virtual bool read(FolderEntry &entry)
   {
      if (pos == children.end()) {
         return false;
      }

      auto node = *(pos++);
      entry.name = node->name;

      if (node->type == FileSystemNode::FileNode) {
         entry.type = FolderEntry::File;
         entry.size = reinterpret_cast<File *>(node)->size(); // TODO: Get file size
      } else if (node->type == FileSystemNode::FolderNode) {
         entry.type = FolderEntry::Folder;
         entry.size = 0;
      }

      return true;
   }

   virtual bool rewind()
   {
      pos = children.begin();
      return true;
   }

   virtual bool close()
   {
      pos = children.end();
      return true;
   }

   virtual FileSystemNode *addFolder(const std::string &name) override
   {
      return addChild(new VirtualFolder(name));
   }

   virtual FileSystemNode *addChild(FileSystemNode *node) override
   {
      children.push_back(node);
      return node;
   }

   virtual FileSystemNode *findChild(const std::string &name) override
   {
      for (auto node : children) {
         if (node->name == name) {
            return node;
         }
      }

      return nullptr;
   }

   std::vector<FileSystemNode *>::iterator pos;
   std::vector<FileSystemNode *> children;
};

} // namespace fs
