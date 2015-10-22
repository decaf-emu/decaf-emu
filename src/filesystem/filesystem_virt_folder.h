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

   bool open() override
   {
      pos = children.begin();
      return true;
   }

   bool read(FolderEntry &entry) override
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

   bool rewind() override
   {
      pos = children.begin();
      return true;
   }

   bool close() override
   {
      pos = children.end();
      return true;
   }

   FileSystemNode *addFolder(const std::string &name) override
   {
      return addChild(new VirtualFolder(name));
   }

   FileSystemNode *addChild(FileSystemNode *node) override
   {
      children.push_back(node);
      return node;
   }

   FileSystemNode *findChild(const std::string &name) override
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
