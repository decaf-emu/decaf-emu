#include "common/platform_memory.h"
#include "common/teenyheap.h"
#include "coreinit.h"
#include "coreinit_memory.h"
#include "coreinit_core.h"
#include "kernel/kernel_memory.h"
#include "libcpu/mem.h"

namespace coreinit
{

namespace internal
{
void initialiseVallocMemory();
}

static const uint32_t VALLOC_PHYS_MEM_START = 0x80000000;
static const uint32_t VALLOC_PHYS_MEM_SIZE = 0x01000000;

static const uint32_t VALLOC_VIRT_MEM_START = 0x88000000;
static const uint32_t VALLOC_VIRT_MEM_SIZE = 0x01000000;

// TODO: This should not be a constant here, but be in libcpu::mem
static const uint32_t SYSTEM_PAGE_SIZE = 0x00001000;

// Our virtual allocation system behaves as though our allocatable physical
//  region is within the 0x80000000-0x81000000 address range.  Additionally,
//  the virtual region which can be allocated is near its location at
//  0x88000000-0x89000000.  This makes debugging slightly easier as each has
//  their own unique address space, and does not collide with real memory.

struct VallocAllocation
{
   uint32_t virtAddress;
   uint32_t physAddress;
   uint32_t size;
};

static uint8_t *
sPhysDataStore = nullptr;

static TeenyHeap *
sVallocVirtualMemHeap = nullptr;

static std::vector<VallocAllocation>
sVallocAllocs;

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
OSGetAvailPhysAddrRange(be_val<ppcaddr_t> *start,
   be_val<uint32_t> *size)
{
   internal::initialiseVallocMemory();

   *start = VALLOC_PHYS_MEM_START;
   *size = VALLOC_PHYS_MEM_SIZE;
}

ppcaddr_t
OSAllocVirtAddr(ppcaddr_t address,
   uint32_t size,
   uint32_t alignment)
{
   internal::initialiseVallocMemory();

   if (address) {
      // This is only a warning, the fact that this method returns an address
      //  rather than a true/false value leads me to believe that it is typically
      //  considered 'acceptable' to not accept the applications suggestion.
      gLog->warn("Application gave us a virtual address, but we ignored it");
   }

   if (!alignment) {
      alignment = 4;
   }

   void *res = sVallocVirtualMemHeap->alloc(size, alignment);

   return mem::untranslate(res);
}

BOOL
OSFreeVirtAddr(ppcaddr_t address,
   uint32_t size)
{
   internal::initialiseVallocMemory();

   // This will assert if the user does not free exactly the address
   //   that a previous call to OSAllocVirtAddr returned.
   sVallocVirtualMemHeap->free(mem::translate(address));

   return TRUE;
}

BOOL
OSMapMemory(ppcaddr_t virtAddress,
   ppcaddr_t physAddress,
   uint32_t size,
   MEMProtectMode mode)
{
   decaf_check(virtAddress);
   decaf_check(physAddress);

   internal::initialiseVallocMemory();

   // First we need to validate the user has not already mapped this
   //  virtual or physical region yet.  Overlaps of virtual addresses
   //  are not currently supported due to high complexity.  Overlaps
   //  of physical memory are not supported because providing memory
   //  coherency is not really possible on Windows.  File Mapping does
   //  not have an acceptable granularity, and none of the windows
   //  memory API's permit mapping the same physical memory to multiple
   //  virtual locations, nor supports reservation, and then later mapping.
   for (auto &alloc : sVallocAllocs) {
      if (!(alloc.virtAddress >= virtAddress + size || alloc.virtAddress + alloc.size <= virtAddress)) {
         decaf_abort("OSMapMemory virtual region overlapped, failing the call");
         return FALSE;
      }
      if (!(alloc.physAddress >= physAddress + size || alloc.physAddress + alloc.size <= physAddress)) {
         gLog->warn("OSMapMemory physical region overlapped, failing the call");
         return FALSE;
      }
   }

   // Protect as R/W so we can write the physical data in
   platform::protectMemory(mem::base() + virtAddress, size, platform::ProtectFlags::ReadWrite);

   // Write the physical data that was already used there
   auto physOffset = physAddress - VALLOC_PHYS_MEM_START;
   memcpy(mem::translate(virtAddress), &sPhysDataStore[physOffset], size);

   // If the application wants read-only, lets do that now.
   if (mode == MEMProtectMode::ReadOnly) {
      platform::protectMemory(mem::base() + virtAddress, size, platform::ProtectFlags::ReadOnly);
   }

   // Store the allocation
   auto alloc = VallocAllocation{ virtAddress, physAddress, size };
   sVallocAllocs.emplace_back(alloc);

   return TRUE;
}

BOOL
OSUnmapMemory(ppcaddr_t virtAddress,
              uint32_t size)
{
   internal::initialiseVallocMemory();

   // Find the relevant allocation record
   auto foundAlloc = sVallocAllocs.end();
   for (auto i = sVallocAllocs.begin(); i != sVallocAllocs.end(); ++i) {
      if (virtAddress >= i->virtAddress && virtAddress < i->virtAddress + i->size) {
         foundAlloc = i;
         break;
      }
   }

   // Make sure we found an actual allocation
   if (foundAlloc == sVallocAllocs.end()) {
      gLog->warn("Application attempted to unmap an unmapped address");
      return FALSE;
   }

   // Our virtual allocation system has some restrictions for now, note that
   //  the code below can still handle partial unmapping, its only the allocation
   //  tracking which cannot currently.
   decaf_check(foundAlloc->virtAddress == virtAddress);
   decaf_check(foundAlloc->size == size);

   // Copy the application's written data out to phys memory again
   auto virtOffset = virtAddress - foundAlloc->virtAddress;
   auto physOffset = foundAlloc->physAddress - VALLOC_PHYS_MEM_START + virtOffset;
   memcpy(&sPhysDataStore[physOffset], mem::translate(virtAddress), size);

   // Re-lock the memory so the application can't touch it
   platform::protectMemory(mem::base() + virtAddress, size, platform::ProtectFlags::NoAccess);

   // Drop the allocation record
   sVallocAllocs.erase(foundAlloc);

   return TRUE;
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
   RegisterKernelFunction(OSAllocVirtAddr);
   RegisterKernelFunction(OSFreeVirtAddr);
   RegisterKernelFunction(OSMapMemory);
   RegisterKernelFunction(OSUnmapMemory);
   RegisterKernelFunctionName("memcpy", coreinit_memcpy);
   RegisterKernelFunctionName("memset", coreinit_memset);
   RegisterKernelFunctionName("memmove", coreinit_memmove);
}

namespace internal
{

   void
   initialiseVallocMemory()
   {
      if (sPhysDataStore) {
         return;
      }

      sPhysDataStore = new uint8_t[VALLOC_PHYS_MEM_SIZE];
      memset(sPhysDataStore, 0, VALLOC_PHYS_MEM_SIZE);

      sVallocVirtualMemHeap =
         new TeenyHeap(mem::translate(VALLOC_VIRT_MEM_START), VALLOC_VIRT_MEM_SIZE);

      platform::commitMemory(
         mem::base() + VALLOC_VIRT_MEM_START,
         VALLOC_VIRT_MEM_SIZE,
         platform::ProtectFlags::NoAccess);
   }

} // namespace internal

} // namespace coreinit
