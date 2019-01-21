#include "vfs_host_directoryiterator.h"
#include "vfs_host_device.h"

namespace vfs
{

HostDirectoryIterator::HostDirectoryIterator(HostDevice *hostDevice,
                                             std::filesystem::directory_iterator itr) :
   mHostDevice(hostDevice),
   mBegin(itr),
   mIterator(std::move(itr))
{
}

Result<Status>
HostDirectoryIterator::readEntry()
{
   if (mIterator == end(mIterator)) {
      return { Error::EndOfDirectory };
   }

   auto ec = std::error_code{ };
   auto currentEntry = Status{ };
   currentEntry.name = mIterator->path().filename().string();

   if (auto size = mIterator->file_size(ec); !ec) {
      currentEntry.size = size;
      currentEntry.flags = Status::HasSize;
   }

   if (mIterator->is_directory()) {
      currentEntry.flags = Status::IsDirectory;
   }

   if (auto result = mHostDevice->lookupPermissions(currentEntry.name); result) {
      currentEntry.group = result->group;
      currentEntry.owner = result->owner;
      currentEntry.permission = result->permission;
      currentEntry.flags = Status::HasPermissions;
   }

   ++mIterator;
   return { currentEntry };
}

Error
HostDirectoryIterator::rewind()
{
   mIterator = mBegin;
   return Error::Success;
}

} // namespace vfs