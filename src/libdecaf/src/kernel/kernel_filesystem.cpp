#include "kernel_filesystem.h"

namespace kernel
{

fs::FileSystem *
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
