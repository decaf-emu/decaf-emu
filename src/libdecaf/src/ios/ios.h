#pragma once
#include "filesystem/filesystem.h"

namespace ios
{

void
start();

void
join();

void
stop();

void
setFileSystem(std::unique_ptr<::fs::FileSystem> fs);

::fs::FileSystem *
getFileSystem();

} // namespace ios
