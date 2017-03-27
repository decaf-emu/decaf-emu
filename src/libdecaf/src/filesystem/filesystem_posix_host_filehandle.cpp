#include "filesystem_host_filehandle.h"
#include <common/platform.h>

#ifdef PLATFORM_POSIX
#include <common/decaf_assert.h>
#include <cstdio>
#include <string>
#include <unistd.h>

namespace fs
{

static std::string
translateMode(File::OpenMode mode)
{
   std::string out;

   if (mode & File::Read) {
      out.push_back('r');
   }

   if (mode & File::Write) {
      out.push_back('w');
   }

   if (mode & File::Append) {
      out.push_back('a');
   }

   out.push_back('b');

   if (mode & File::Update) {
      out.push_back('+');
   }

   return out;
}


HostFileHandle::HostFileHandle(const std::string &path, File::OpenMode mode) :
   mMode(mode)
{
   auto hostMode = translateMode(mode);
   mHandle = fopen(path.c_str(), hostMode.c_str());
}


bool
HostFileHandle::open()
{
   return !!mHandle;
}


void
HostFileHandle::close()
{
   if (mHandle) {
      fclose(mHandle);
   }

   mHandle = nullptr;
}


bool
HostFileHandle::eof()
{
   decaf_check(mHandle);
   return !!feof(mHandle);
}


bool
HostFileHandle::flush()
{
   decaf_check(mHandle);
   return !!fflush(mHandle);
}


bool
HostFileHandle::seek(size_t position)
{
   decaf_check(mHandle);
   return fseeko(mHandle, position, SEEK_SET) == 0;
}


size_t
HostFileHandle::tell()
{
   decaf_check(mHandle);
   auto pos = ftello(mHandle);

   decaf_check(pos >= 0)
   return static_cast<size_t>(pos);
}


size_t
HostFileHandle::size()
{
   decaf_check(mHandle);
   auto pos = tell();

   fseeko(mHandle, 0, SEEK_END);
   auto length = tell();

   seek(pos);
   return length;
}


size_t
HostFileHandle::truncate()
{
   decaf_check(mHandle);
   decaf_check((mMode & File::Write) || (mMode & File::Update));
   auto length = size();

   if (ftruncate(fileno(mHandle), static_cast<off_t>(length))) {
      return 0;
   }

   return length;
}


size_t
HostFileHandle::read(uint8_t *data,
                     size_t size,
                     size_t count)
{
   decaf_check(mHandle);
   decaf_check((mMode & File::Read) || (mMode & File::Update));
   return fread(data, size, count, mHandle);
}


size_t
HostFileHandle::write(const uint8_t *data,
                      size_t size,
                      size_t count)
{
   decaf_check(mHandle);
   decaf_check((mMode & File::Write) || (mMode & File::Update));
   return fwrite(data, size, count, mHandle);
}

} // namespace fs

#endif // ifdef PLATFORM_POSIX
