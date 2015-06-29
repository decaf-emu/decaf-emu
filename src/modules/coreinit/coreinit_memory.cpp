#include "coreinit.h"
#include "coreinit_memory.h"
#include "memory.h"

void *
OSBlockMove(void *dst, const void *src, size_t size, BOOL flush)
{
   std::memmove(dst, src, size);
   return dst;
}

void *
OSBlockSet(void *dst, uint8_t val, size_t size)
{
   std::memset(dst, val, size);
   return dst;
}

static void *
coreinit_memmove(void *dst, const void *src, size_t size)
{
   std::memmove(dst, src, size);
   return dst;
}

static void *
coreinit_memcpy(void *dst, const void *src, size_t size)
{
   std::memcpy(dst, src, size);
   return dst;
}

static void *
coreinit_memset(void *dst, int val, size_t size)
{
   std::memset(dst, val, size);
   return dst;
}

int
OSGetMemBound(OSMemoryType type, uint32_t *addr, uint32_t *size)
{
   switch (type) {
   case OSMemoryType::MEM1:
      *addr = byte_swap<uint32_t>(0xf4000000);
      *size = byte_swap<uint32_t>(0x02000000);
      break;
   case OSMemoryType::MEM2:
      *addr = byte_swap<uint32_t>(0x02000000);
      *size = byte_swap<uint32_t>(0x40000000);
      break;
   case OSMemoryType::System:
      *addr = byte_swap<uint32_t>(0x01000000);
      *size = byte_swap<uint32_t>(0x01000000);
      break;
   default:
      return -1;
   }

   return 0;
}

// First 40mb of foreground is for applications
// Last 24mb of foreground is saved for system use
BOOL
OSGetForegroundBucket(uint32_t *addr, uint32_t *size)
{
   // TODO: Return true if calling process is in foreground area
   *addr = byte_swap<uint32_t>(0xe0000000);
   *size = byte_swap<uint32_t>(0x04000000);
   return TRUE;
}

BOOL
OSGetForegroundBucketFreeArea(uint32_t *addr, uint32_t *size)
{
   // TODO: Return true if calling process is in foreground area
   *addr = byte_swap<uint32_t>(0xe0000000);
   *size = byte_swap<uint32_t>(0x02800000);
   return TRUE;
}

void
CoreInit::registerMemoryFunctions()
{
   RegisterSystemFunction(OSBlockMove);
   RegisterSystemFunction(OSBlockSet);
   RegisterSystemFunction(OSGetMemBound);
   RegisterSystemFunction(OSGetForegroundBucket);
   RegisterSystemFunction(OSGetForegroundBucketFreeArea);
   RegisterSystemFunctionName("memcpy", coreinit_memcpy);
   RegisterSystemFunctionName("memset", coreinit_memset);
   RegisterSystemFunctionName("memmove", coreinit_memmove);
}
