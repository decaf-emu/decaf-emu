#include "platform.h"
#include "platform_memory.h"
#include "log.h"

#ifdef PLATFORM_POSIX
#include <common/decaf_assert.h>
#include <cstdlib>
#include <errno.h>
#include <fmt/format.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace platform
{

static int
flagsToProt(ProtectFlags flags)
{
   switch (flags) {
   case ProtectFlags::ReadOnly:
      return PROT_READ;
   case ProtectFlags::ReadWrite:
      return PROT_READ | PROT_WRITE;
   case ProtectFlags::ReadExecute:
      return PROT_READ | PROT_EXEC;
   case ProtectFlags::ReadWriteExecute:
      return PROT_READ | PROT_WRITE | PROT_EXEC;
   case ProtectFlags::NoAccess:
   default:
      return PROT_WRITE;
   }
}

static int
flagsToOpen(ProtectFlags flags)
{
   switch (flags) {
   case ProtectFlags::ReadOnly:
      return O_RDONLY;
   case ProtectFlags::ReadWrite:
      return O_RDWR;
   case ProtectFlags::ReadExecute:
      return O_RDONLY;
   case ProtectFlags::ReadWriteExecute:
      return O_RDWR;
   case ProtectFlags::NoAccess:
   default:
      return PROT_WRITE;
   }
}


size_t
getSystemPageSize()
{
   return static_cast<size_t>(sysconf(_SC_PAGESIZE));
}


MapFileHandle
createMemoryMappedFile(size_t size)
{
   const char *tmpdir = getenv("TMPDIR");
   if (!tmpdir || !*tmpdir) {
      tmpdir = "/tmp";
   }
   const std::string pattern = fmt::format("{}/decafXXXXXX", tmpdir);
   char *path = strdup(pattern.c_str());  // Must be a modifiable char array.
   int old_umask = umask(0077);
   int fd = mkstemp(path);
   if (fd == -1) {
      gLog->error("createMemoryMappedFile({}) mkstemp failed with error: {}",
                  size, errno);
      umask(old_umask);
      return InvalidMapFileHandle;
   }
   umask(old_umask);

   if (unlink(path) == -1) {
      gLog->error("createMemoryMappedFile({}) unlink failed with error: {}",
                  size, errno);
   }

   free(path);

#ifdef PLATFORM_APPLE
   if (ftruncate(fd, size) == -1) {
#else
   if (ftruncate64(fd, size) == -1) {
#endif
      gLog->error("createMemoryMappedFile({}) ftruncate64 failed with error: {}",
                  size, errno);
   }

   return static_cast<MapFileHandle>(fd);
}


MapFileHandle
openMemoryMappedFile(const std::string &path,
                     ProtectFlags flags,
                     size_t *outSize)
{
   // Only support READ ONLY for now
   decaf_check(flags == ProtectFlags::ReadOnly);
   struct stat st;
   if (stat(path.c_str(), &st) == -1) {
      gLog->error("openMemoryMappedFile(\"{}\") stat failed with error: {}",
                  path, errno);
      return InvalidMapFileHandle;
   }

   auto fd = open(path.c_str(), flagsToOpen(flags), 0);
   if (fd == -1) {
      gLog->error("openMemoryMappedFile(\"{}\") open failed with error: {}",
                  path, errno);
      return InvalidMapFileHandle;
   }

   *outSize = st.st_size;
   return static_cast<MapFileHandle>(fd);
}


bool
closeMemoryMappedFile(MapFileHandle handle)
{
   if (close(static_cast<int>(handle)) == -1) {
      gLog->error("closeMemoryMappedFile({}) close failed with error: {}",
                  static_cast<int>(handle), errno);
      return false;
   }

   return true;
}


void *
mapViewOfFile(MapFileHandle handle,
              ProtectFlags flags,
              size_t offset,
              size_t size,
              void *dst)
{
   auto prot = flagsToProt(flags);
   auto result = mmap(dst,
                      size,
                      prot,
                      MAP_SHARED,
                      static_cast<int>(handle),
                      offset);

   if (result == MAP_FAILED) {
      gLog->error("mapViewOfFile(offset: 0x{:X}, size: 0x{:X}, dst: {}) mmap failed with error: {}",
                  offset, size, dst, errno);
      return nullptr;
   }

   if (result != dst) {
      gLog->error("mapViewOfFile(offset: 0x{:X}, size: 0x{:X}, dst: {}) mmap returned unexpected address: {}",
                  offset, size, dst, result);

      munmap(result, size);
      return nullptr;
   }

   return result;
}


bool
unmapViewOfFile(void *view,
                size_t size)
{
   if (munmap(view, size) == -1) {
      gLog->error("unmapViewOfFile(view: 0x{:X}, size: 0x{:X}) munmap failed with error: {}",
                  view, size, errno);
      return false;
   }

   return true;
}


bool
reserveMemory(uintptr_t address,
              size_t size)
{
   // On *nix systems, regions mapped with mmap are reserved only by default
   //  and become automatically commited on first use.  Because these pages
   //  have no protection rights, they are forced to stay reserved.
   auto baseAddress = reinterpret_cast<void *>(address);
   auto result = mmap(baseAddress,
                      size,
                      PROT_NONE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1,
                      0);

   if (result == MAP_FAILED) {
      gLog->debug("reserveMemory(address: 0x{:08X}, size: 0x{:X}) mmap failed with error: {}",
                  address, size, errno);
      return false;
   }

   if (result != baseAddress) {
      gLog->debug("reserveMemory(address: 0x{:08X}, size: 0x{:X}) returned unexpected address: {}",
                  address, size, result);

      munmap(result, size);
      return false;
   }

   return true;
}


bool
freeMemory(uintptr_t address,
           size_t size)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   if (munmap(baseAddress, size) == -1) {
      gLog->error("freeMemory(address: 0x{:08X}, size: 0x{:X}) munmap failed with error: {}",
                  address, size, errno);
      return false;
   }

   return true;
}


bool
commitMemory(uintptr_t address,
             size_t size,
             ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<void *>(address);

   if (mprotect(baseAddress, size, flagsToProt(flags)) == -1) {
      gLog->error("commitMemory(address: 0x{:08X}, size: 0x{:X}, flags: {}) mprotect failed with error: {}",
                  address, size, static_cast<int>(flags), errno);
      return false;
   }

   return true;
}


bool
uncommitMemory(uintptr_t address,
               size_t size)
{
   // On *nix systems, there is not really a way to forcibly uncommit
   //   a particular region of code.  We just lock it out.
   auto baseAddress = reinterpret_cast<void *>(address);

   if (mprotect(baseAddress, size, PROT_NONE) == -1) {
      gLog->error("uncommitMemory(address: {}, size: {}) mprotect failed with error: {}",
                  address, size, errno);
      return false;
   }

   return true;
}


bool
protectMemory(uintptr_t address,
              size_t size,
              ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<void *>(address);

   if (mprotect(baseAddress, size, flagsToProt(flags)) == -1) {
      gLog->error("protectMemory(address: {}, size: {}, flags: {}) mprotect failed with error: {}",
                  address, size, static_cast<int>(flags), errno);
      return false;
   }

   return true;
}

} // namespace platform

#endif
