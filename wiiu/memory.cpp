#include "memory.h"

Memory gMemory;

void Memory::initialise()
{
   mVirtualAddress = 0x02000000;
   mMemorySize = 0x40000000; // 1GB
   mAddress = reinterpret_cast<size_t>(VirtualAlloc(0, 0x40000000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));

   if (!mAddress) {
      xError() << "Could not allocate virtual memory " << GetLastError();
   }
}

size_t Memory::translate(size_t address)
{
   // TODO: Proper memory map
   if (address >= ApplicationCode && address < ApplicationData) {
   } else if (address >= ApplicationData && address < ApplicationMemoryEnd) {
   } else if (address >= GraphicsResources && address <= GraphicsResourcesEnd) {
   } else {
      return 0;
   }

   assert(address >= mVirtualAddress);
   return (address - mVirtualAddress) + mAddress;
}

size_t Memory::allocate(size_t address, size_t size)
{
   return translate(address);
}

void Memory::free(size_t address)
{
   translate(address);
}
