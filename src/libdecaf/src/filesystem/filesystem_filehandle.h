#pragma once
#include <cstddef>
#include <cstdint>

namespace fs
{

class FileHandle
{
public:
   virtual ~FileHandle() = default;

   virtual bool open() = 0;
   virtual void close() = 0;

   virtual bool flush() = 0;
   virtual size_t truncate() = 0;

   virtual bool seek(size_t position) = 0;
   virtual size_t tell() = 0;
   virtual bool eof() = 0;

   virtual size_t size() = 0;

   virtual size_t read(uint8_t *data, size_t size, size_t count) = 0;
   virtual size_t read(uint8_t *data, size_t size, size_t count, size_t position) = 0;

   virtual size_t write(uint8_t *data, size_t size, size_t count) = 0;
   virtual size_t write(uint8_t *data, size_t size, size_t count, size_t position) = 0;
};

} // namespace fs
