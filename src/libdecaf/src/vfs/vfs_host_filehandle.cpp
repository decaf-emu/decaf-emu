#include "vfs_host_filehandle.h"

#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#include <io.h>
#elif defined(PLATFORM_POSIX)
#include <unistd.h>
#include <sys/types.h>
#endif

namespace vfs
{

HostFileHandle::HostFileHandle(FILE *handle, Mode mode) :
   mHandle(handle),
   mMode(mode)
{
}

HostFileHandle::~HostFileHandle()
{
   close();
}

Error
HostFileHandle::close()
{
   if (mHandle) {
      fclose(mHandle);
      mHandle = nullptr;
   }

   return Error::Success;
}

Result<bool>
HostFileHandle::eof()
{
   if (!mHandle) {
      return { Error::NotOpen };
   }

   return { !!feof(mHandle) };
}

Error
HostFileHandle::flush()
{
   if (!mHandle) {
      return Error::NotOpen;
   }

   fflush(mHandle);
   return Error::Success;
}

Error
HostFileHandle::seek(SeekDirection direction, int64_t offset)
{
   if (!mHandle) {
      return Error::NotOpen;
   }

   int seekDirection = SEEK_SET;
   switch (direction) {
   case SeekCurrent:
      seekDirection = SEEK_CUR;
      break;
   case SeekEnd:
      seekDirection = SEEK_END;
      break;
   case SeekStart:
      seekDirection = SEEK_SET;
      break;
   default:
      return Error::InvalidSeekDirection;
   }


#ifdef PLATFORM_WINDOWS
   if (_fseeki64(mHandle, offset, seekDirection) != 0) {
      // TODO: Translate ERRNO ?
      return Error::GenericError;
   }
#elif defined(PLATFORM_POSIX)
   if (fseeko(mHandle, offset, seekDirection) != 0) {
      // TODO: Translate ERRNO ?
      return Error::GenericError;
   }
#else
   return Error::OperationNotSupported;
#endif

   return Error::Success;
}

Result<int64_t>
HostFileHandle::size()
{
   if (!mHandle) {
      return { Error::NotOpen };
   }

   auto position = tell();
   if (!position) {
      return { position.error() };
   }

   auto error = seek(SeekEnd, 0);
   if (error != Error::Success) {
      return { position.error() };
   }

   auto size = tell();
   if (!size) {
      return { size.error() };
   }

   seek(SeekStart, *position);
   return size;
}

Result<int64_t>
HostFileHandle::tell()
{
   if (!mHandle) {
      return Error::NotOpen;
   }

#ifdef PLATFORM_WINDOWS
   auto position = _ftelli64(mHandle);
   if (position < 0) {
      // TODO: Translate error?
      return { Error::GenericError };
   }
#elif defined(PLATFORM_POSIX)
   auto position = ftello(mHandle);
   if (position < 0) {
      // TODO: Translate error?
      return { Error::GenericError };
   }
#else
   return Error::OperationNotSupported;
#endif

   return { static_cast<int64_t>(position) };
}

Result<int64_t>
HostFileHandle::truncate()
{
   if (!mHandle) {
      return { Error::NotOpen };
   }

   // TODO: Check open mode for read only

   auto length = tell();
   if (!length) {
      return length.error();
   }

#ifdef PLATFORM_WINDOWS
   if (_chsize_s(_fileno(mHandle), static_cast<long long>(*length)) != 0) {
      // TODO: Translate error?
      return { Error::GenericError };
   }
#elif defined(PLATFORM_POSIX)
   fflush(mHandle);
   if (ftruncate(fileno(mHandle), static_cast<off_t>(*length)) != 0) {
      // TODO: Translate error?
      return { Error::GenericError };
   }
#else
   return Error::OperationNotSupported;
#endif

   return length;
}

Result<int64_t>
HostFileHandle::read(void *buffer, int64_t size, int64_t count)
{
   auto result = static_cast<int64_t>(fread(buffer, size, count, mHandle));
   if (result != size * count) {
      auto eof = feof(mHandle);
      auto err = ferror(mHandle);
      printf("eof: %d, err: %d\n", eof, err);
   }
   return { result };
}

Result<int64_t>
HostFileHandle::write(const void *buffer, int64_t size, int64_t count)
{
   if (!mHandle) {
      return { Error::NotOpen };
   }

   // TODO: Check open mode for read only
   return { static_cast<int64_t>(fwrite(buffer, size, count, mHandle)) };
}

} // namespace vfs
