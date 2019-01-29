#include "filesystem_host_filehandle.h"
#include <common/platform.h>

#ifdef PLATFORM_WINDOWS
#include <common/decaf_assert.h>
#include <common/platform_winapi_string.h>

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <io.h>

namespace fs
{

static std::wstring
translateMode(File::OpenMode mode)
{
   std::wstring out;

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


HostFileHandle::HostFileHandle(const std::string &path,
                               File::OpenMode mode) :
   mMode(mode)
{
   auto hostMode = translateMode(mode);
   auto hostPath = platform::toWinApiString(path);

   _wfopen_s(&mHandle, hostPath.c_str(), hostMode.c_str());
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
   return _fseeki64(mHandle, position, SEEK_SET) == 0;
}


size_t
HostFileHandle::tell()
{
   decaf_check(mHandle);
   auto pos = _ftelli64(mHandle);

   decaf_check(pos >= 0)
   return static_cast<size_t>(pos);
}


size_t
HostFileHandle::size()
{
   decaf_check(mHandle);
   auto pos = tell();

   _fseeki64(mHandle, 0, SEEK_END);
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

   if (_chsize_s(_fileno(mHandle), static_cast<long long>(length))) {
      return 0;
   }

   return length;
}


size_t
HostFileHandle::read(void *data,
                     size_t size,
                     size_t count)
{
   decaf_check(mHandle);
   decaf_check((mMode & File::Read) || (mMode & File::Update));
   return fread_s(data, size * count, size, count, mHandle);
}


size_t
HostFileHandle::write(const void *data,
                      size_t size,
                      size_t count)
{
   decaf_check(mHandle);
   decaf_check((mMode & File::Write) ||
               (mMode & File::Update) ||
               (mMode & File::Append));
   return fwrite(data, size, count, mHandle);
}

} // namespace fs

#endif // ifdef PLATFORM_WINDOWS
