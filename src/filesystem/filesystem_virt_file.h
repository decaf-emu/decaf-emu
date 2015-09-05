#pragma once
#include "filesystem_file.h"

namespace fs
{

struct VirtualFile : File
{
   VirtualFile(std::string name) :
      File(name, FileSystemNode::VirtualNode)
   {
   }

   virtual ~VirtualFile()
   {
   }

   virtual bool open(OpenMode mode) override
   {
      return false;
   }

   virtual size_t size() override
   {
      return 0;
   }

   virtual size_t tell() override
   {
      return 0;
   }

   virtual size_t seek(size_t pos) override
   {
      return 0;
   }

   virtual size_t read(char *buffer, size_t size) override
   {
      return 0;
   }

   virtual size_t read(char *buffer, size_t size, size_t pos) override
   {
      return 0;
   }

   virtual size_t write(char *buffer, size_t size) override
   {
      return 0;
   }

   virtual size_t write(char *buffer, size_t size, size_t pos) override
   {
      return 0;
   }

   virtual void close() override
   {
   }
};

} // namespace fs
