#pragma once
#include "filesystem.h"

namespace fs
{

struct File : FileSystemNode
{
   File(std::string name, FileSystemNode::HostType hostType) :
      FileSystemNode(FileSystemNode::FileNode, hostType, name)
   {
   }

   virtual ~File()
   {
   }

   enum OpenMode
   {
      Read     = 1 << 0,
      Write    = 1 << 1,
      Append   = 1 << 2,
      Update   = 1 << 3
   };

   virtual bool open(OpenMode mode) = 0;
   virtual void close() = 0;

   virtual size_t size() = 0;

   virtual size_t tell() = 0;
   virtual size_t seek(size_t pos) = 0;

   virtual size_t read(char *buffer, size_t size) = 0;
   virtual size_t read(char *buffer, size_t size, size_t pos) = 0;

   virtual size_t write(char *buffer, size_t size) = 0;
   virtual size_t write(char *buffer, size_t size, size_t pos) = 0;
};

} // namespace fs
