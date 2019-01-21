#include "vfs_virtual_file.h"
#include "vfs_virtual_filehandle.h"

#include <algorithm>
#include <cstring>

namespace vfs
{

VirtualFileHandle::VirtualFileHandle(std::shared_ptr<VirtualFile> file,
                                     Mode mode) :
   mFile(std::move(file)),
   mMode(mode),
   mPosition(0)
{
}

VirtualFileHandle::~VirtualFileHandle()
{
   close();
}

Error
VirtualFileHandle::close()
{
   mFile.reset();
   return Error::Success;
}

Result<bool>
VirtualFileHandle::eof()
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   return mPosition >= static_cast<int64_t>(mFile->data.size());
}

Error
VirtualFileHandle::flush()
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   return Error::Success;
}

Error
VirtualFileHandle::seek(SeekDirection direction,
                        int64_t offset)
{
   if (!mFile) {
      return Error::NotOpen;
   }

   auto oldPosition = mPosition;
   switch (direction) {
   case SeekCurrent:
      mPosition += offset;
      break;
   case SeekEnd:
      mPosition = static_cast<int64_t>(mFile->data.size()) + offset;
      break;
   case SeekStart:
      mPosition = offset;
      break;
   default:
      return Error::InvalidSeekDirection;
   }

   if (mPosition < 0) {
      // Cannot seek before start of file!
      mPosition = oldPosition;
      return Error::InvalidSeekPosition;
   }

   return Error::Success;
}

Result<int64_t>
VirtualFileHandle::size()
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   return { static_cast<int64_t>(mFile->data.size()) };
}

Result<int64_t>
VirtualFileHandle::tell()
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   return { mPosition };
}

Result<int64_t>
VirtualFileHandle::truncate()
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   mFile->data.resize(static_cast<size_t>(mPosition));
   return { mPosition };
}

Result<int64_t>
VirtualFileHandle::read(void *buffer,
                        int64_t size,
                        int64_t count)
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   if (mPosition >= static_cast<int64_t>(mFile->data.size())) {
      return { Error::EndOfFile };
   }

   auto groupsRemaining = (static_cast<int64_t>(mFile->data.size()) - mPosition) / size;
   count = std::min(count, groupsRemaining);
   if (count == 0) {
      return { 0 };
   }

   std::memcpy(buffer, mFile->data.data() + mPosition, size * count);
   mPosition += size * count;
   return { count };
}

Result<int64_t>
VirtualFileHandle::write(const void *buffer,
                         int64_t size,
                         int64_t count)
{
   if (!mFile) {
      return { Error::NotOpen };
   }

   if (mMode & Append) {
      mPosition = static_cast<int64_t>(mFile->data.size());
   }

   auto endPosition = mPosition + size * count;
   if (endPosition > static_cast<int64_t>(mFile->data.size())) {
      mFile->data.resize(static_cast<size_t>(endPosition));
   }

   std::memcpy(mFile->data.data() + mPosition, buffer, size * count);
   mPosition += size * count;
   return { count };
}

} // namespace vfs
