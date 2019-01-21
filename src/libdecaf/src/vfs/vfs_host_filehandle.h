#pragma once
#include "vfs_filehandle.h"

#include <cstdio>

namespace vfs
{

class HostFileHandle : public FileHandle
{
public:
   HostFileHandle(FILE *handle, Mode mode);
   ~HostFileHandle() override;

   Error close() override;
   Result<bool> eof() override;
   Error flush() override;
   Error seek(SeekDirection direction, int64_t offset) override;
   Result<int64_t> size() override;
   Result<int64_t> tell() override;
   Result<int64_t> truncate() override;
   Result<int64_t> read(void *buffer, int64_t size, int64_t count) override;
   Result<int64_t> write(const void *buffer, int64_t size, int64_t count) override;

private:
   FILE *mHandle;
   Mode mMode;
};

} // namespace vfs
