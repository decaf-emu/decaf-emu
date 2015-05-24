#pragma once
#include <cassert>
#include <Windows.h>
#include "bitutils.h"
#include "log.h"

enum MemoryMap : size_t
{
   ApplicationCodeStart = 0x02000000,
   ApplicationCodeEnd = 0x10000000,
   ApplicationDataStart = 0x10000000,
   ApplicationDataEnd = 0x02000000 + 0x40000000, // 1GB
   ForegroundAreaStart = 0xE0000000,
   ForegroundAreaEnd = 0xE4000000,
   MEM1Start = ForegroundAreaStart,
   MEM1End = ForegroundAreaEnd,
   GraphicsResourcesStart = 0xF4000000,
   GraphicsResourcesEnd = 0xF6000000
};

class Memory
{
   struct Allocation
   {
      size_t physical;
      size_t size;
   };

public:
   void initialise();
   size_t translate(size_t address);
   size_t allocate(size_t address, size_t size);
   void free(size_t address);

   template<typename Type>
   Type read(size_t address)
   {
      return byte_swap(*reinterpret_cast<Type*>(translate(address)));
   }

   template<typename Type>
   Type readNoSwap(size_t address)
   {
      return *reinterpret_cast<Type*>(translate(address));
   }

   template<typename Type>
   void write(size_t address, Type value)
   {
      *reinterpret_cast<Type*>(translate(address)) = byte_swap(value);
   }

   template<typename Type>
   void writeNoSwap(size_t address, Type value)
   {
      *reinterpret_cast<Type*>(translate(address)) = value;
   }

private:
   size_t mVirtualAddress;
   size_t mAddress;
   size_t mMemorySize;
};

extern Memory gMemory;
