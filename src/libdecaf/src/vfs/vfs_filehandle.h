#pragma once
#include "vfs_result.h"

#include <cstdint>

namespace vfs
{

class FileHandle
{
public:
   enum Mode
   {
      Read     = 1 << 0,
      Write    = 1 << 1,
      Append   = 1 << 2,
      Update   = 1 << 3,
      Execute  = 1 << 4,
   };

   enum SeekDirection
   {
      SeekStart,
      SeekCurrent,
      SeekEnd,
   };

   virtual ~FileHandle() = default;

   virtual Error close() = 0;
   virtual Result<bool> eof() = 0;
   virtual Error flush() = 0;
   virtual Error seek(SeekDirection direction, int64_t position) = 0;
   virtual Result<int64_t> size() = 0;
   virtual Result<int64_t> tell() = 0;
   virtual Result<int64_t> truncate() = 0;
   virtual Result<int64_t> read(void *buffer, int64_t size, int64_t count) = 0;
   virtual Result<int64_t> write(const void *buffer, int64_t size, int64_t count) = 0;
};

} // namespace vfs
