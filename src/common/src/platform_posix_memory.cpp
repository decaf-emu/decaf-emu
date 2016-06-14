#include "platform.h"
#include "platform_memory.h"

#ifdef PLATFORM_POSIX
#include <sys/mman.h>

namespace platform
{

static int flagsToProt(ProtectFlags flags) {
   switch (flags) {
   case ProtectFlags::ReadOnly:
      return PROT_READ;
   case ProtectFlags::ReadWrite:
      return PROT_READ | PROT_WRITE;
   case ProtectFlags::ReadExecute:
      return PROT_READ | PROT_EXECUTE;
   case ProtectFlags::ReadWriteExecute:
      return PROT_READ | PROT_WRITE | PROT_EXECUTE;
   case ProtectFlags::NoAccess:
   default:
      return PROT_WRITE;
   }
}

bool
reserveMemory(size_t address, size_t size)
{
   // On *nix systems, regions mapped with mmap are reserved only by default
   //  and become automatically commited on first use.  Because these pages
   //  have no protection rights, they are forced to stay reserved.
   auto baseAddress = reinterpret_cast<void *>(address);
   auto result = mmap(baseAddress, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

   if (result != baseAddress) {
      if (result != nullptr) {
         freeMemory(address, size);
      }
      return false;
   }

   return true;
}

bool
freeMemory(size_t address, size_t size)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   return munmap(baseAddress, size) == 0;
}

bool
commitMemory(size_t address, size_t size, ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   return mprotect(baseAddress, size, flagsToProt(flags)) == 0;
}

bool
uncommitMemory(size_t address, size_t size)
{
   // On *nix systems, there is not really a way to forcibly uncommit
   //   a particular region of code.  We just lock it out.
   auto baseAddress = reinterpret_cast<void *>(address);
   return mprotect(baseAddress, size, PROT_NONE);
}

bool
protectMemory(size_t address, size_t size, ProtectFlags flags)
{
   auto baseAddress = reinterpret_cast<void *>(address);
   return mprotect(baseAddress, size, flagsToProt(flags)) == 0;
}

} // namespace platform

#endif
