#pragma once
#include <fstream>
#include "filesystem_file.h"
#include "filesystem_win_path.h"

namespace fs
{

struct HostFile : public File
{
   HostFile(std::string name, std::string hostPath) :
      File(name, FileSystemNode::HostNode),
      path(hostPath)
   {
   }

   virtual ~HostFile()
   {
   }

   virtual bool open(OpenMode mode) override
   {
      handle.open(path.path(), convertMode(mode));
      return handle.is_open();
   }

   virtual size_t size() override
   {
      if (handle.is_open()) {
         auto pos = handle.tellg();
         handle.seekg(0, std::ifstream::end);
         auto size = handle.tellg();
         handle.seekg(pos);
         return size;
      } else if (open(OpenMode::Read)) {
         auto size = this->size();
         close();
         return size;
      }

      return 0;
   }

   virtual size_t tell() override
   {
      return handle.tellg();
   }

   virtual size_t seek(size_t pos) override
   {
      handle.seekg(pos);
      return tell();
   }

   virtual size_t read(char *buffer, size_t size) override
   {
      handle.read(buffer, size);
      return handle.gcount();
   }

   virtual size_t read(char *buffer, size_t size, size_t pos) override
   {
      seek(pos);
      return read(buffer, size);
   }

   virtual size_t write(char *buffer, size_t size) override
   {
      auto before = handle.tellp();
      handle.write(buffer, size);
      auto after = handle.tellp();
      return after - before;
   }

   virtual size_t write(char *buffer, size_t size, size_t pos) override
   {
      seek(pos);
      return write(buffer, size);
   }

   virtual void close() override
   {
      handle.close();
   }

   std::ios_base::openmode convertMode(OpenMode mode)
   {
      std::ios_base::openmode openMode = std::fstream::binary;

      if (mode & OpenMode::Read) {
         openMode |= std::fstream::in;
      }

      if (mode & OpenMode::Write) {
         openMode |= std::fstream::in;
      }

      if (mode & OpenMode::Append) {
         openMode |= std::fstream::app;
      }

      // How does + mode work?
      assert(!(mode & OpenMode::Update));
      return openMode;
   }

   HostPath path;
   std::fstream handle;
};

} // namespace fs
