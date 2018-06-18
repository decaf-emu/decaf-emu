#include "kernel_filesystem.h"

namespace kernel
{

static fs::FileSystem *
sFileSystem = nullptr;

void
setFileSystem(fs::FileSystem *fs)
{
   sFileSystem = fs;
}

fs::FileSystem *
getFileSystem()
{
   return sFileSystem;
}

} // namespace kernel
