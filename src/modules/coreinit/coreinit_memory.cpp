#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_core.h"
#include "mem/mem.h"

namespace coreinit
{

// TODO: These values need to reset after game is unloaded
static uint32_t
sMem1Start = mem::MEM1Base;

static uint32_t
sMem1Size = mem::MEM1Size;

static uint32_t
sMem2Start = mem::ApplicationBase;

static uint32_t
sMem2Size = mem::ApplicationSize;

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

int
OSGetMemBound(OSMemoryType type, be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   switch (type) {
   case OSMemoryType::MEM1:
      *addr = sMem1Start;
      *size = sMem1Size;
      break;
   case OSMemoryType::MEM2:
      *addr = sMem2Start;
      *size = sMem2Size;
      break;
   default:
      return -1;
   }

   return 0;
}

BOOL
OSGetForegroundBucket(be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   if (addr) {
      *addr = mem::ForegroundBase;
   }

   if (size) {
      *size = mem::ForegroundSize;
   }

   return TRUE;
}

BOOL
OSGetForegroundBucketFreeArea(be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   if (addr) {
      *addr = mem::ForegroundBase;
   }

   if (size) {
      // First 40mb of foreground is for applications
      // Last 24mb of foreground is saved for system use
      *size = 40 * 1024 * 1024;
   }

   return TRUE;
}

void
OSMemoryBarrier()
{
   // TODO: OSMemoryBarrier
}

void
Module::registerMemoryFunctions()
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

namespace internal
{

int
setMemBound(OSMemoryType type, uint32_t start, uint32_t size)
{
   switch (type) {
   case OSMemoryType::MEM1:
      sMem1Start = start;
      sMem1Size = size;
      break;
   case OSMemoryType::MEM2:
      sMem2Start = start;
      sMem2Size = size;
      break;
   default:
      return -1;
   }

   return 0;
}

} // namespace internal

} // namespace coreinit
