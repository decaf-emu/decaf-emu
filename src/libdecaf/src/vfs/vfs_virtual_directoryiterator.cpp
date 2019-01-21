#include "vfs_virtual_directory.h"
#include "vfs_virtual_directoryiterator.h"
#include "vfs_virtual_file.h"

#include <system_error>

namespace vfs
{

VirtualDirectoryIterator::VirtualDirectoryIterator(std::shared_ptr<VirtualDirectory> directory) :
   mDirectory(std::move(directory)),
   mIterator(mDirectory->begin())
{
}

Result<Status>
VirtualDirectoryIterator::readEntry()
{
   if (mIterator == mDirectory->end()) {
      return Error::EndOfDirectory;
   }

   auto ec = std::error_code { };
   auto entry = Status{ };
   entry.name = mIterator->first;

   auto node = mIterator->second.get();
   if (node->type == VirtualNode::File) {
      auto file = static_cast<VirtualFile *>(node);
      entry.size = file->data.size();
      entry.flags = Status::HasSize;
   }

   if (node->type == VirtualNode::Directory) {
      entry.flags = Status::IsDirectory;
   }

   entry.group = node->group;
   entry.owner = node->owner;
   entry.permission = node->permission;
   entry.flags = Status::HasPermissions;
   return entry;
}

Error
VirtualDirectoryIterator::rewind()
{
   mIterator = mDirectory->begin();
   return Error::Success;
}

} // namespace vfs
