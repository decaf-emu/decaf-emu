#pragma once
#include "filesystem/filesystem.h"
#include <memory>

namespace kernel
{

void
setFileSystem(std::unique_ptr<fs::FileSystem> fs);

fs::FileSystem *
getFileSystem();

} // namespace kernel
