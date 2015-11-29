#pragma once
#include <fstream>
#include "filesystem_file.h"
#include "filesystem_filehandle.h"

namespace fs
{

struct HostFileHandle : public FileHandle
{
   HostFileHandle(const std::string &path, File::OpenMode mode)
   {
      std::ios_base::openmode iosMode = std::fstream::binary;

      if (mode & File::Read) {
         iosMode |= std::fstream::in;
      }

      if (mode & File::Write) {
         iosMode |= std::fstream::out;
      }

      if (mode & File::Append) {
         iosMode |= std::fstream::app;
      }

      mHandle.open(path, iosMode);
   }

   virtual ~HostFileHandle()
   {
   }

   virtual bool open() override
   {
      return mHandle.is_open();
   }

   virtual void close() override
   {
      mHandle.close();
   }

   virtual bool flush() override
   {
      mHandle.flush();
      return true;
   }

   virtual size_t truncate() override
   {
      // TODO: Implement truncate
      return 0;
   }

   virtual bool seek(size_t position) override
   {
      mHandle.seekg(position);
      mHandle.seekp(position);
      return true;
   }

   virtual size_t tell() override
   {
      return static_cast<size_t>(mHandle.tellg());
   }

   virtual size_t size() override
   {
      auto pos = mHandle.tellg();
      mHandle.seekg(0, mHandle.end);
      auto result = static_cast<size_t>(mHandle.tellg());
      mHandle.seekg(pos);
      return result;
   }

   virtual size_t read(uint8_t *data, size_t size) override
   {
      mHandle.read(reinterpret_cast<char *>(data), size);
      return static_cast<size_t>(mHandle.gcount());
   }

   virtual size_t read(uint8_t *data, size_t size, size_t position) override
   {
      auto previous = tell();
      seek(position);
      auto result = read(data, size);
      seek(previous);
      return result;
   }

   virtual size_t write(uint8_t *data, size_t size) override
   {
      mHandle.write(reinterpret_cast<const char *>(data), size);
      return size;
   }

   virtual size_t write(uint8_t *data, size_t size, size_t position) override
   {
      auto previous = tell();
      seek(position);
      auto result = write(data, size);
      seek(previous);
      return result;
   }

private:
   std::fstream mHandle;
};

} // namespace fs
