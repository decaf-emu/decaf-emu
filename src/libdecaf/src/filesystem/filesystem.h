#pragma once
#include "filesystem_error.h"
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include "filesystem_host_folder.h"
#include "filesystem_host_path.h"
#include "filesystem_link_file.h"
#include "filesystem_link_folder.h"
#include "filesystem_path.h"
#include "filesystem_result.h"
#include "filesystem_virtual_folder.h"

#include <common/log.h>
#include <memory>

namespace fs
{

class FileSystem
{
public:
   FileSystem() :
      mRoot(Permissions::ReadWrite, "/")
   {
   }

   Result<Folder *>
   makeFolder(Path path)
   {
      auto result = createPath(path);
      auto node = result.value();

      if (!result || node->type() != Node::FolderNode) {
         return Error::GenericError;
      }

      return reinterpret_cast<Folder *>(node);
   }

   Result<Error>
   remove(Path path)
   {
      auto parent = findNode(path.parentPath());

      if (!parent || parent->type() != Node::FolderNode) {
         return Error::NotFound;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return folder->remove(path.filename());
   }

   Result<Error>
   move(Path src,
        Path dst)
   {
      // Find our parents
      auto srcParent = findNode(src.parentPath());

      if (!srcParent) {
         return Error::NotFound;
      }

      auto result = createPath(dst.parentPath());
      auto dstParent = result.value();

      if (!result) {
         return Error::NotFound;
      }

      // For now we only support moving within HostFolder
      if (srcParent->type() != Node::FolderNode && srcParent->deviceType() != Node::HostDevice) {
         return Error::UnsupportedOperation;
      }

      if (dstParent->type() != Node::FolderNode && dstParent->deviceType() != Node::HostDevice) {
         return Error::UnsupportedOperation;
      }

      // Request the HostFolder to perform the move
      auto srcFolder = reinterpret_cast<HostFolder *>(srcParent);
      auto dstFolder = reinterpret_cast<HostFolder *>(dstParent);

      return HostFolder::move(srcFolder, src.filename(), dstFolder, dst.filename());
   }

   Result<Node *>
   makeLink(Path dst,
            Path src)
   {
      return makeLink(dst, findNode(src));
   }

   Result<Node *>
   makeLink(Path dst,
            Node *srcNode)
   {
      // Ensure src exists
      if (!srcNode) {
         return { Error::NotFound };
      }

      // Check to see if dst already exists
      auto dstNode = findNode(dst);

      if (dstNode) {
         return { Error::AlreadyExists };
      }

      // Create parent path
      auto result = createPath(dst.parentPath());

      if (!result) {
         return result;
      }

      auto parent = result.value();

      if (parent->type() != Node::FolderNode || parent->deviceType() != Node::VirtualDevice) {
         return { Error::NotDirectory };
      }

      auto folder = reinterpret_cast<VirtualFolder *>(parent);

      // Create link
      if (srcNode->type() == Node::FolderNode) {
         dstNode = new FolderLink { reinterpret_cast<Folder *>(srcNode), dst.filename() };
      } else if (srcNode->type() == Node::FileNode) {
         dstNode = new FileLink { reinterpret_cast<File *>(srcNode), dst.filename() };
      }

      if (dstNode) {
         dstNode = folder->addChild(dstNode);
      }

      return dstNode;
   }

   Result<Node *>
   mountHostFolder(Path dst,
                   HostPath src,
                   Permissions permissions)
   {
      auto result = createPath(dst.parentPath());

      if (!result) {
         return  result;
      }

      auto parent = result.value();

      if (parent->type() != Node::FolderNode || parent->deviceType() != Node::VirtualDevice) {
         return { Error::NotDirectory };
      }

      gLog->debug("Mount {} to {}", src.path(), dst.path());
      auto folder = reinterpret_cast<VirtualFolder *>(parent);
      auto name = dst.filename();
      return folder->addChild(new HostFolder { src, name, permissions });
   }

   Result<Node *>
   mountHostFile(Path dst,
                 HostPath src,
                 Permissions permissions)
   {
      auto result = createPath(dst.parentPath());

      if (!result) {
         return result;
      }

      auto parent = result.value();

      if (parent->type() != Node::FolderNode || parent->deviceType() != Node::VirtualDevice) {
         return { Error::NotDirectory };
      }

      auto folder = reinterpret_cast<VirtualFolder *>(parent);
      return folder->addChild(new HostFile { src, dst.filename(), permissions });
   }

   Result<FileHandle>
   openFile(Path path,
            File::OpenMode mode)
   {
      auto parent = findNode(path.parentPath());

      if (!parent || parent->type() != Node::FolderNode) {
         return Error::NotFound;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return folder->openFile(path.filename(), mode);
   }

   Result<FolderHandle>
   openFolder(Path path)
   {
      auto parent = findNode(path);

      if (!parent || parent->type() != Node::FolderNode) {
         return Error::NotFound;
      }

      auto folder = reinterpret_cast<Folder *>(parent);
      return folder->openDirectory();
   }

   Result<FolderEntry>
   findEntry(Path path)
   {
      auto node = findNode(path);

      if (!node) {
         return { Error::NotFound };
      }

      FolderEntry entry;
      entry.name = node->name();
      entry.size = node->size();

      if (node->type() == Node::FileNode) {
         entry.type = FolderEntry::File;
      } else if (node->type() == Node::FolderNode) {
         entry.type = FolderEntry::Folder;
      } else {
         entry.type = FolderEntry::Unknown;
      }

      return entry;
   }

   bool
   setPermissions(Path path,
                  Permissions permissions,
                  PermissionFlags flags)
   {
      auto node = findNode(path);

      if (!node) {
         return false;
      }

      node->setPermissions(permissions, flags);
      return true;
   }

protected:
   Node *
   followLink(Node *node)
   {
      if (node && node->deviceType() == Node::LinkDevice) {
         if (node->type() == Node::FileNode) {
            return reinterpret_cast<FileLink *>(node)->getLink();
         } else if (node->type() == Node::FolderNode) {
            return reinterpret_cast<FolderLink *>(node)->getLink();
         }
      }

      return node;
   }

   Node *
   findNode(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type() != Node::FolderNode) {
            return nullptr;
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         auto folder = reinterpret_cast<Folder *>(node);
         node = folder->findChild(dir);
      }

      return followLink(node);
   }

   Result<Node *>
   createPath(const Path &path)
   {
      auto node = reinterpret_cast<Node *>(&mRoot);

      for (auto dir : path) {
         if (!node || node->type() != Node::FolderNode) {
            return { Error::NotDirectory };
         }

         // Skip root directory
         if (node == &mRoot && dir.compare("/") == 0) {
            continue;
         }

         // Try create the folder
         auto folder = reinterpret_cast<Folder *>(node);
         auto result = folder->addFolder(dir);

         if (result != Error::OK && result != Error::AlreadyExists) {
            return { result.error() };
         }

         node = result.value();
      }

      return node;
   }

private:
   VirtualFolder mRoot;
};

} // namespace fs
