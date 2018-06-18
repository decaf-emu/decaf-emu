#pragma once
#include "filesystem/filesystem.h"
#include <memory>

namespace ios
{

void
start();

void
join();

void
setFileSystem(std::unique_ptr<fs::FileSystem> fs);

} // namespace ios
