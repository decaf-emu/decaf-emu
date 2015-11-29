#pragma once
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include "filesystem_host_folder.h"
#include "filesystem_host_path.h"
#include "filesystem_path.h"
#include "filesystem_virtual_folder.h"

namespace fs
{

class FileSystem
{
public:
   FileSystem() :
      mRoot("/")
   {
   }

   bool mountHostFolder(Path dst, HostPath src)
   {
      auto parent = createVirtualPath(dst.parentPath());

      if (!parent || parent->type != Node::FolderNode) {
         return false;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return !!folder->addChild(new HostFolder(src, dst.filename()));
   }

   FileHandle *openFile(Path path, File::OpenMode mode)
   {
      auto node = findNode(path);

      if (!node || node->type != Node::FileNode) {
         return false;
      }

      auto file = reinterpret_cast<File *>(node);
      return file->open(mode);
   }

   FolderHandle *openFolder(Path path)
   {
      auto node = findNode(path);

      if (!node || node->type != Node::FolderNode) {
         return false;
      }

      auto file = reinterpret_cast<Folder *>(node);
      return file->open();
   }

   bool findEntry(Path path, FolderEntry &entry)
   {
      auto node = findNode(path);

      if (!node) {
         return false;
      }

      entry.name = node->name;
      entry.size = node->size;

      if (node->type == Node::FileNode) {
         entry.type = FolderEntry::File;
      } else if (node->type == Node::FolderNode) {
         entry.type = FolderEntry::Folder;
      } else {
         entry.type = FolderEntry::Unknown;
      }

      return true;
   }

protected:
   Node *findNode(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type != Node::FolderNode) {
            return nullptr;
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         auto folder = reinterpret_cast<Folder *>(node);
         node = folder->findChild(dir);
      }

      return node;
   }

   Node *createVirtualPath(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type != Node::FolderNode) {
            return nullptr;
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         auto folder = reinterpret_cast<Folder *>(node);
         node = folder->addFolder(dir);
      }

      return node;
   }

private:
   VirtualFolder mRoot;
};

} // namespace fs
