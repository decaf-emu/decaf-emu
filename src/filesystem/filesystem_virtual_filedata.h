#pragma once
#include <mutex>
#include <vector>

namespace fs
{

struct VirtualFileData
{
   std::vector<char> data;
   std::mutex mutex;
};

} // namespace fs
