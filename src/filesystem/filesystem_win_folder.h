#pragma once
#include <Windows.h>
#include "filesystem_folder.h"
#include "filesystem_virt_folder.h"

namespace fs
{

// Lazy populate with real folders / files as requested
struct HostFolder : public Folder
{
   HostFolder(std::string name, std::string path) :
      Folder(name, FileSystemNode::HostNode),
      path(path),
      virtualFolder(name),
      findHandle(INVALID_HANDLE_VALUE)
   {
   }

   virtual ~HostFolder()
   {
      close();
   }

   virtual bool open()
   {
      findHandle = FindFirstFileA(path.join("*"), &findData);
      return findHandle != INVALID_HANDLE_VALUE;
   }

   virtual bool read(FolderEntry &entry)
   {
      if (findHandle == INVALID_HANDLE_VALUE) {
         return false;
      }

      entry.name = findData.cFileName;

      if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         entry.type = FolderEntry::Folder;
         entry.size = 0;
      } else {
         entry.type = FolderEntry::File;
         entry.size = findData.nFileSizeLow;
      }

      if (!FindNextFileA(findHandle, &findData)) {
         close();
      }

      if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0) {
         // Skip over . and .. entries
         return read(entry);
      }

      return true;
   }

   virtual bool rewind()
   {
      return close() && open();
   }

   virtual bool close()
   {
      if (findHandle != INVALID_HANDLE_VALUE) {
         FindClose(findHandle);
         findHandle = INVALID_HANDLE_VALUE;
      }

      return true;
   }

   virtual FileSystemNode *addFolder(const std::string &name) override
   {
      auto node = findChild(name);

      // Check if child already exists
      if (node) {
         if (node->type == FileSystemNode::FolderNode) {
            // Directory exists, return it
            return node;
         } else {
            // Non-directory with same name, fuck.
            return nullptr;
         }
      }

      // Create a directory
      auto hostPath = path.join(name);

      if (!CreateDirectoryA(hostPath, NULL)) {
         return nullptr;
      }

      // Add a virtual entry
      return addChild(new HostFolder(name, hostPath));
   }

   virtual FileSystemNode *addChild(FileSystemNode *node) override
   {
      // Ensure child is a Host type
      if (node->hostType != FileSystemNode::HostNode) {
         assert(node->hostType == FileSystemNode::HostNode);
         return nullptr;
      }

      virtualFolder.addChild(node);
      return node;
   }

   virtual FileSystemNode *findChild(const std::string &name) override
   {
      // Find file/folder in virtual file system
      auto node = virtualFolder.findChild(name);

      if (node) {
         return node;
      }

      // Find file/folder in host file system
      WIN32_FIND_DATAA fileData;
      auto hostPath = path.join(name);
      auto handle = FindFirstFileA(hostPath, &fileData);

      if (handle == INVALID_HANDLE_VALUE) {
         // File not found
         return nullptr;
      }

      // File/Directory found, create matching virtual entry
      if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         node = addChild(new HostFolder(fileData.cFileName, hostPath));
      } else {
         node = addChild(new HostFile(fileData.cFileName, hostPath));
      }

      FindClose(handle);
      return node;
   }

   HostPath path;
   VirtualFolder virtualFolder;
   WIN32_FIND_DATAA findData;
   HANDLE findHandle;
};

} // namespace fs
