#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_core.h"
#include "kernel/kernel_memory.h"
#include "libcpu/mem.h"

namespace coreinit
{

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
   if (val < 0 || val > 0xFF) {
      gLog->warn("Application called memset with non-uint8_t value");
   }

   std::memset(dst, val, size);
   return dst;
}

int
OSGetMemBound(OSMemoryType type, be_val<uint32_t> *addr, be_val<uint32_t> *size)
{
   uint32_t memAddr, memSize;

   switch (type) {
   case OSMemoryType::MEM1:
      kernel::getMEM1Bound(&memAddr, &memSize);
      break;
   case OSMemoryType::MEM2:
      kernel::getMEM2Bound(&memAddr, &memSize);
      break;
   default:
      return -1;
   }

   *addr = memAddr;
   *size = memSize;
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

} // namespace coreinit
