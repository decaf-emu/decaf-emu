#pragma once
#include "common/decaf_assert.h"
#include "filesystem_file.h"
#include "filesystem_filehandle.h"
#include <fstream>
#include <cstring>

namespace fs
{

struct HostFileHandle : public FileHandle
{
   HostFileHandle(const std::string &path, File::OpenMode mode)
   {
      char openModeStr[6] = "";

      if (mode & File::Read) {
         strcat(openModeStr, "r");
      }

      if (mode & File::Write) {
         strcat(openModeStr, "w");
      }

      if (mode & File::Append) {
         strcat(openModeStr, "a");
      }

      strcat(openModeStr, "b");

      if (mode & File::Update) {
         strcat(openModeStr, "+");
      }

      mHandle = fopen(path.c_str(), openModeStr);
   }

   virtual ~HostFileHandle() override = default;

   virtual bool open() override
   {
      return mHandle != NULL;
   }

   virtual void close() override
   {
      fclose(mHandle);
      mHandle = NULL;
   }

   virtual bool flush() override
   {
      return fflush(mHandle) == 0;
   }

   virtual size_t truncate() override
   {
      // TODO: Implement truncate
      return 0;
   }

   virtual bool seek(size_t position) override
   {
      return fseek(mHandle, position, SEEK_SET) == 0;
   }

   virtual bool eof() override
   {
      return feof(mHandle) != 0;
   }

   virtual size_t tell() override
   {
      return ftell(mHandle);
   }

   virtual size_t size() override
   {
      auto pos = tell();
      fseek(mHandle, 0, SEEK_END);
      auto size = tell();
      seek(pos);
      return size;
   }

   virtual size_t read(uint8_t *data, size_t size, size_t count) override
   {
      return fread(data, size, count, mHandle);
   }

   virtual size_t read(uint8_t *data, size_t size, size_t count, size_t position) override
   {
      seek(position);
      return read(data, size, count);
   }

   virtual size_t write(uint8_t *data, size_t size, size_t count) override
   {
      return fwrite(data, size, count, mHandle);
   }

   virtual size_t write(uint8_t *data, size_t size, size_t count, size_t position) override
   {
      seek(position);
      return write(data, size, count);
   }

private:
   FILE *mHandle;

};

} // namespace fs
