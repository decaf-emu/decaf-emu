#pragma once
#include "vfs_directoryiterator.h"
#include <filesystem>

namespace vfs
{

class HostDevice;

class HostDirectoryIterator : public DirectoryIteratorImpl
{
public:
   HostDirectoryIterator(HostDevice *hostDevice,
                         std::filesystem::directory_iterator itr);
   ~HostDirectoryIterator() override = default;

   Result<Status> readEntry() override;
   Error rewind() override;

private:
   class HostDevice *mHostDevice;
   std::filesystem::directory_iterator mBegin;
   std::filesystem::directory_iterator mIterator;
};

} // namespace vfs
