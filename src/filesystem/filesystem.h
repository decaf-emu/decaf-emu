#pragma once
#include <cassert>
#include "filesystem_node.h"
#include "filesystem_path.h"
#include "filesystem_host_file.h"
#include "filesystem_host_folder.h"
#include "filesystem_virt_file.h"
#include "filesystem_virt_folder.h"

namespace fs
{

class FileSystem
{
public:
   FileSystem() :
      mRoot("/")
   {
   }

   // Add node at path, will create path if not exists
   bool addNode(const FilePath &path, FileSystemNode *node)
   {
      auto parent = createParentPath(path);

      if (!parent) {
         return false;
      }

      assert(parent->type == FileSystemNode::FolderNode);
      auto folder = reinterpret_cast<Folder *>(parent);
      folder->addChild(node);
      return true;
   }

   // Mount hostPath folder at path
   bool mountHostFolder(const FilePath &path, std::string hostPath)
   {
      return addNode(path, new HostFolder(path.name(), hostPath));
   }

   // Create whole path
   FileSystemNode *createPath(const FilePath &path)
   {
      auto itr = path.exploded.begin();
      auto end = path.exploded.end();

      if (mRoot.name == *(itr++)) {
         if (itr == end) {
            return &mRoot;
         } else {
            return mRoot.createPath(itr, end);
         }
      } else {
         return nullptr;
      }
   }

   // Create path leading up to the top level directory/file in FilePath
   FileSystemNode *createParentPath(const FilePath &path)
   {
      auto itr = path.exploded.begin();
      auto end = path.exploded.end() - 1;

      if (mRoot.name == *(itr++)) {
         if (itr == end) {
            return &mRoot;
         } else {
            return mRoot.createPath(itr, end);
         }
      } else {
         return nullptr;
      }
   }

   // Find a node at a path
   FileSystemNode *findNode(const FilePath &path)
   {
      auto itr = path.exploded.begin();
      auto end = path.exploded.end();

      if (mRoot.name == *(itr++)) {
         return mRoot.findNode(itr, end);
      } else {
         return nullptr;
      }
   }

   File *findFile(const FilePath &path)
   {
      auto node = findNode(path);

      if (!node || node->type != FileSystemNode::FileNode) {
         return nullptr;
      } else {
         return reinterpret_cast<File *>(node);
      }
   }

   Folder *findFolder(const FilePath &path)
   {
      auto node = findNode(path);

      if (!node || node->type != FileSystemNode::FolderNode) {
         return nullptr;
      } else {
         return reinterpret_cast<Folder *>(node);
      }
   }

   File *openFile(const FilePath &path, fs::File::OpenMode mode)
   {
      auto file = findFile(path);

      if (!file || !file->open(mode)) {
         return nullptr;
      } else {
         return file;
      }
   }

   Folder *openFolder(const FilePath &path)
   {
      auto folder = findFolder(path);

      if (!folder || !folder->open()) {
         return nullptr;
      } else {
         return folder;
      }
   }

private:
   VirtualFolder mRoot;
};

} // namespace fs
