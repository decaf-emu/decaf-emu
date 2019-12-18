#include "vfs_host_directoryiterator.h"
#include "vfs_host_device.h"

namespace vfs
{

HostDirectoryIterator::HostDirectoryIterator(HostDevice *hostDevice,
                                             std::vector<Status> listing) :
   mHostDevice(hostDevice),
   mListing(std::move(listing)),
   mIterator(mListing.begin())
{
}

Result<Status>
HostDirectoryIterator::readEntry()
{
   if (mIterator == mListing.end()) {
      return { Error::EndOfDirectory };
   }

   auto currentEntry = *mIterator;
   ++mIterator;
   return { currentEntry };
}

Error
HostDirectoryIterator::rewind()
{
   mIterator = mListing.begin();
   return Error::Success;
}

} // namespace vfs