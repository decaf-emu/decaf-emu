#include "coreinit.h"
#include "coreinit_memory.h"
#include "memory.h"

void *
OSBlockMove(void *dst, const void *src, ppcsize_t size, BOOL flush)
{
   std::memmove(dst, src, size);
   return dst;
}

void *
OSBlockSet(void *dst, uint8_t val, ppcsize_t size)
{
   std::memset(dst, val, size);
   return dst;
}

static void *
coreinit_memmove(void *dst, const void *src, ppcsize_t size)
{
   std::memmove(dst, src, size);
   return dst;
}

static void *
coreinit_memcpy(void *dst, const void *src, ppcsize_t size)
{
   std::memcpy(dst, src, size);
   return dst;
}

static void *
coreinit_memset(void *dst, int val, ppcsize_t size)
{
   std::memset(dst, val, size);
   return dst;
}

uint32_t gMem1Start = 0xf4000000;
uint32_t gMem1Size  = 0x02000000;

uint32_t gMem2Start = 0x02000000;
uint32_t gMem2Size  = 0x40000000;

int
OSGetMemBound(OSMemoryType type, be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   switch (type) {
   case OSMemoryType::MEM1:
      *addr = gMem1Start;
      *size = gMem1Size;
      break;
   case OSMemoryType::MEM2:
      *addr = gMem2Start;
      *size = gMem2Size;
      break;
   default:
      return -1;
   }

   return 0;
}

int
OSSetMemBound(OSMemoryType type, uint32_t start, uint32_t size)
{
   switch (type) {
   case OSMemoryType::MEM1:
      gMem1Start = start;
      gMem1Size = size;
      break;
   case OSMemoryType::MEM2:
      gMem2Start = start;
      gMem2Size = size;
      break;
   default:
      return -1;
   }

   return 0;
}

// First 40mb of foreground is for applications
// Last 24mb of foreground is saved for system use
BOOL
OSGetForegroundBucket(be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   if (addr) {
      *addr = 0xe0000000;
   }

   if (size) {
      *size = 0x04000000;
   }

   return TRUE;
}

BOOL
OSGetForegroundBucketFreeArea(be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   if (addr) {
      *addr = 0xe0000000;
   }

   if (size) {
      *size = 0x02800000;
   }

   return TRUE;
}

void
OSMemoryBarrier()
{
   // TODO: OSMemoryBarrier
}

void
CoreInit::registerMemoryFunctions()
{
   RegisterKernelFunction(OSBlockMove);
   RegisterKernelFunction(OSBlockSet);
   RegisterKernelFunction(OSGetMemBound);
   RegisterKernelFunction(OSGetForegroundBucket);
   RegisterKernelFunction(OSGetForegroundBucketFreeArea);
   RegisterKernelFunction(OSMemoryBarrier);
   RegisterKernelFunctionName("memcpy", coreinit_memcpy);
   RegisterKernelFunctionName("memset", coreinit_memset);
   RegisterKernelFunctionName("memmove", coreinit_memmove);
}
