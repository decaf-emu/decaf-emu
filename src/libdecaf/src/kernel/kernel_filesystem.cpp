#include "kernel_filesystem.h"

namespace kernel
{

static std::unique_ptr<fs::FileSystem>
sFileSystem = nullptr;

void
setFileSystem(std::unique_ptr<fs::FileSystem> fs)
{
   sFileSystem = std::move(fs);
}

fs::FileSystem *
getFileSystem()
{
   return sFileSystem.get();
}

} // namespace kernel
