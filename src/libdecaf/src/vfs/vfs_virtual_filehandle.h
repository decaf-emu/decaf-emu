#pragma once
#include "vfs_filehandle.h"

#include <memory>

namespace vfs
{

struct VirtualFile;

class VirtualFileHandle : public FileHandle
{
public:
   VirtualFileHandle(std::shared_ptr<VirtualFile> file,
                     Mode mode);
   ~VirtualFileHandle() override;

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
   std::shared_ptr<VirtualFile> mFile;
   int64_t mPosition;
   Mode mMode;
};

} // namespace vfs
