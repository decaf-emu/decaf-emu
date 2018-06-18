#pragma once
#include "filesystem/filesystem.h"

namespace kernel
{

void
setFileSystem(fs::FileSystem *fs);

fs::FileSystem *
getFileSystem();

} // namespace kernel
