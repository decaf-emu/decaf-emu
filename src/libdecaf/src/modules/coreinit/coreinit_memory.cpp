#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_core.h"
#include "kernel/kernel_memory.h"
#include "kernel/kernel_syscalls.h"

#include <common/platform_memory.h>
#include <common/teenyheap.h>
#include <libcpu/mmu.h>

namespace coreinit
{

void *
OSBlockMove(void *dst,
            const void *src,
            uint32_t size,
            BOOL flush)
{
   std::memmove(dst, src, size);
   return dst;
}

void *
OSBlockSet(void *dst,
           uint8_t val,
           uint32_t size)
{
   std::memset(dst, val, size);
   return dst;
}

static void *
coreinit_memmove(void *dst,
                 const void *src,
                 uint32_t size)
{
   std::memmove(dst, src, size);
   return dst;
}

static void *
coreinit_memcpy(void *dst,
                const void *src,
                uint32_t size)
{
   std::memcpy(dst, src, size);
   return dst;
}

static void *
coreinit_memset(void *dst,
                int val,
                uint32_t size)
{
   std::memset(dst, val, size);
   return dst;
}

int
OSGetMemBound(OSMemoryType type,
              be_val<uint32_t> *addr,
              be_val<uint32_t> *size)
{
   uint32_t memAddr, memSize;

   switch (type) {
   case OSMemoryType::MEM1:
   {
      auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::MEM1);
      memAddr = bounds.start.getAddress();
      memSize = bounds.size;
      break;
   }
   case OSMemoryType::MEM2:
   {
      auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::MainAppData);
      memAddr = bounds.start.getAddress();
      memSize = bounds.size;
      break;
   }
   default:
      return -1;
   }

   *addr = memAddr;
   *size = memSize;
   return 0;
}

BOOL
OSGetForegroundBucket(be_val<uint32_t> *addr,
                      be_val<uint32_t> *size)
{
   auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::ForegroundBucket);

   if (addr) {
      *addr = bounds.start.getAddress();
   }

   if (size) {
      *size = bounds.size;
   }

   return TRUE;
}

BOOL
OSGetForegroundBucketFreeArea(be_val<uint32_t> *addr,
                              be_val<uint32_t> *size)
{
   auto bounds = kernel::getVirtualRange(kernel::VirtualRegion::ForegroundBucket);

   if (addr) {
      *addr = bounds.start.getAddress();
   }

   if (size) {
      // First 40mb of foreground is for applications
      // Last 24mb of foreground is saved for system use
      *size = 40 * 1024 * 1024;
      decaf_check(bounds.size >= *size);
   }

   return TRUE;
}

void
OSMemoryBarrier()
{
   // TODO: OSMemoryBarrier
}

void
OSGetAvailPhysAddrRange(be_val<ppcaddr_t> *start,
                        be_val<uint32_t> *size)
{
   auto range = kernel::syscall::getAvailPhysAddrRange();

   if (start) {
      *start = range.start.getAddress();
   }

   if (size) {
      *size = range.size;
   }
}

void
OSGetDataPhysAddrRange(be_val<ppcaddr_t> *start,
                       be_val<uint32_t> *size)
{
   auto range = kernel::syscall::getDataPhysAddrRange();

   if (start) {
      *start = range.start.getAddress();
   }

   if (size) {
      *size = range.size;
   }
}

void
OSGetMapVirtAddrRange(be_val<ppcaddr_t> *start,
                      be_val<uint32_t> *size)
{
   auto range = kernel::syscall::getMapVirtAddrRange();

   if (start) {
      *start = range.start.getAddress();
   }

   if (size) {
      *size = range.size;
   }
}

ppcaddr_t
OSAllocVirtAddr(ppcaddr_t address,
                uint32_t size,
                uint32_t alignment)
{
   return kernel::syscall::allocVirtAddr(cpu::VirtualAddress { address },
                                         size,
                                         alignment).getAddress();
}

BOOL
OSFreeVirtAddr(ppcaddr_t address,
               uint32_t size)
{
   return kernel::syscall::freeVirtAddr(cpu::VirtualAddress { address },
                                        size)
         ? TRUE : FALSE;
}

kernel::VirtualMemoryType
OSQueryVirtAddr(ppcaddr_t address)
{
   return kernel::syscall::queryVirtAddr(cpu::VirtualAddress { address });
}

BOOL
OSMapMemory(ppcaddr_t virtAddress,
            ppcaddr_t physAddress,
            uint32_t size,
            kernel::MapPermission permission)
{
   return kernel::syscall::mapMemory(cpu::VirtualAddress { virtAddress },
                                     cpu::PhysicalAddress { physAddress },
                                     size,
                                     permission)
         ? TRUE : FALSE;
}

BOOL
OSUnmapMemory(ppcaddr_t virtAddress,
              uint32_t size)
{
   return kernel::syscall::unmapMemory(cpu::VirtualAddress { virtAddress },
                                       size)
         ? TRUE : FALSE;
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
   RegisterKernelFunction(OSGetAvailPhysAddrRange);
   RegisterKernelFunction(OSGetDataPhysAddrRange);
   RegisterKernelFunction(OSGetMapVirtAddrRange);
   RegisterKernelFunction(OSAllocVirtAddr);
   RegisterKernelFunction(OSFreeVirtAddr);
   RegisterKernelFunction(OSQueryVirtAddr);
   RegisterKernelFunction(OSMapMemory);
   RegisterKernelFunction(OSUnmapMemory);
   RegisterKernelFunctionName("memcpy", coreinit_memcpy);
   RegisterKernelFunctionName("memset", coreinit_memset);
   RegisterKernelFunctionName("memmove", coreinit_memmove);
}

} // namespace coreinit
