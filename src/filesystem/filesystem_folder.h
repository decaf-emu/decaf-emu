#pragma once
#include "filesystem_node.h"

namespace fs
{

struct FolderEntry
{
   enum Type
   {
      File,
      Folder
   };

   Type type;
   std::string name;
   size_t size;
};

struct Folder : FileSystemNode
{
   Folder(std::string name, FileSystemNode::HostType hostType) :
      FileSystemNode(FileSystemNode::FolderNode, hostType, name)
   {
   }

   virtual ~Folder()
   {
   }

   virtual FileSystemNode *addChild(FileSystemNode *node) = 0;
   virtual FileSystemNode *addFolder(const std::string &name) = 0;

   virtual FileSystemNode *findChild(const std::string &name) = 0;

   virtual bool open() = 0;
   virtual bool read(FolderEntry &entry) = 0;
   virtual bool rewind() = 0;
   virtual bool close() = 0;

   FileSystemNode *createPath(FilePathIterator path, FilePathIterator end)
   {
      auto &name = *(path++);
      auto child = findChild(name);

      if (child) {
         if (path == end) {
            if (child->type == FileSystemNode::FolderNode) {
               return child;
            } else {
               return nullptr;
            }
         } else if (child->type != FileSystemNode::FolderNode) {
            return nullptr;
         }

         return reinterpret_cast<Folder *>(child)->createPath(path, end);
      }

      child = addFolder(name);

      if (path == end) {
         return child;
      } else {
         return reinterpret_cast<Folder *>(child)->createPath(path, end);
      }
   }

   FileSystemNode *findNode(FilePathIterator path, FilePathIterator end)
   {
      auto &name = *(path++);
      auto node = findChild(name);

      if (!node) {
         return nullptr;
      } else if (path == end) {
         return node;
      } else if (node->type != FileSystemNode::FolderNode) {
         return nullptr;
      } else {
         return reinterpret_cast<Folder *>(node)->findNode(path, end);
      }
   }
};

} // namespace fs
