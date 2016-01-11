#pragma once
#include <algorithm>
#include <vector>
#include "filesystem_filehandle.h"
#include "filesystem_file.h"
#include "filesystem_virtual_filedata.h"

namespace fs
{

struct VirtualFileHandle : public FileHandle
{
   VirtualFileHandle(VirtualFileData &data, fs::File::OpenMode mode) :
      mData(data),
      mMode(mode),
      mPosition(0)
   {
      if (mode & fs::File::Append) {
         mPosition = mData.data.size();
      }

      if (mode & fs::File::Write) {
         mData.data.resize(0);
      }
   }

   virtual ~VirtualFileHandle() override = default;

   virtual bool open() override
   {
      return true;
   }

   virtual void close() override
   {
   }

   virtual bool flush() override
   {
      return true;
   }

   virtual size_t truncate() override
   {
      std::lock_guard<std::mutex> lock { mData.mutex };
      mData.data.resize(mPosition);
      return 0;
   }

   virtual bool seek(size_t position) override
   {
      mPosition = position;
      return true;
   }

   virtual bool eof() override
   {
      return tell() >= size();
   }

   virtual size_t tell() override
   {
      return mPosition;
   }

   virtual size_t size() override
   {
      std::lock_guard<std::mutex> lock { mData.mutex };
      return mData.data.size();
   }

   virtual size_t read(uint8_t *data, size_t size, size_t count) override
   {
      std::lock_guard<std::mutex> lock { mData.mutex };

      auto bytes = size * count;
      auto start = mPosition;
      auto end = std::min(mPosition + bytes, mData.data.size());
      bytes = end - start;
      count = bytes / size;

      std::memcpy(data, mData.data.data() + start, bytes);
      mPosition = end;
      return count;
   }

   virtual size_t read(uint8_t *data, size_t size, size_t count, size_t position) override
   {
      auto previous = tell();
      seek(position);
      auto result = read(data, size, count);
      seek(previous);
      return result;
   }

   virtual size_t write(uint8_t *data, size_t size, size_t count) override
   {
      std::lock_guard<std::mutex> lock { mData.mutex };

      // In append mode, all writes set position to end of file
      if (mMode & fs::File::Append) {
         mPosition = mData.data.size();
      }

      auto bytes = size * count;
      auto start = mPosition;
      auto end = mPosition + bytes;

      if (end > mData.data.size()) {
         mData.data.resize(end);
      }

      std::memcpy(mData.data.data() + start, data, bytes);
      mPosition = end;
      return count;
   }

   virtual size_t write(uint8_t *data, size_t size, size_t count, size_t position) override
   {
      auto previous = tell();
      seek(position);
      auto result = write(data, size, count);
      seek(previous);
      return result;
   }

private:
   size_t mPosition;
   fs::File::OpenMode mMode;
   VirtualFileData &mData;
};

} // namespace fs
