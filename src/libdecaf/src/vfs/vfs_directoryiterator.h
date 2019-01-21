#pragma once
#include "vfs_error.h"
#include "vfs_result.h"
#include "vfs_status.h"

#include <memory>

namespace vfs
{

struct DirectoryIteratorImpl
{
   virtual ~DirectoryIteratorImpl() = default;
   virtual Result<Status> readEntry() = 0;
   virtual Error rewind() = 0;
};

class DirectoryIterator
{
public:
   DirectoryIterator() = default;
   DirectoryIterator(std::shared_ptr<DirectoryIteratorImpl> impl) :
      mImpl(impl)
   {
   }

   Result<Status> readEntry()
   {
      return mImpl->readEntry();
   }

   Error rewind()
   {
      return mImpl->rewind();
   }

private:
   std::shared_ptr<DirectoryIteratorImpl> mImpl;
};

} // namespace vfs
