#pragma once
#include "vfs_directoryiterator.h"
#include "vfs_virtual_directory.h"

#include <memory>

namespace vfs
{

class VirtualDirectoryIterator : public DirectoryIteratorImpl
{
public:
   VirtualDirectoryIterator(std::shared_ptr<VirtualDirectory> directory);
   ~VirtualDirectoryIterator() override = default;

   Result<Status> readEntry() override;
   Error rewind() override;

private:
   std::shared_ptr<VirtualDirectory> mDirectory;
   VirtualDirectory::iterator mIterator;
};

} // namespace vfs
