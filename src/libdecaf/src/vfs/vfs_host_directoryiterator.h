#pragma once
#include "vfs_directoryiterator.h"

#include <filesystem>
#include <vector>

namespace vfs
{

class HostDevice;

class HostDirectoryIterator : public DirectoryIteratorImpl
{
public:
   HostDirectoryIterator(HostDevice *hostDevice,
                         std::vector<Status> listing);
   ~HostDirectoryIterator() override = default;

   Result<Status> readEntry() override;
   Error rewind() override;

private:
   class HostDevice *mHostDevice;
   std::vector<Status> mListing;
   std::vector<Status>::iterator mIterator;
};

} // namespace vfs
