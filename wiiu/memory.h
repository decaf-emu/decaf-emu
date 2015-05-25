#pragma once
#include <cassert>
#include <vector>
#include <Windows.h>
#include "bitutils.h"
#include "log.h"

union PageEntry
{
   struct
   {
      uint32_t base : 20;           // First page in region
      uint32_t count : 20;          // Number of pages in region (only valid in base page)
      uint32_t allocated : 1;       // Is page allocated?
      uint32_t : 22;
   };

   uint64_t value = 0;
};

struct MemoryView
{
   MemoryView() :
      address(nullptr)
   {
   }

   MemoryView(uint64_t start, uint64_t end, uint64_t pageSize) :
      start(start), end(end), pageSize(pageSize), address(nullptr)
   {
   }

   uint64_t start;
   uint64_t end;
   uint8_t *address;
   size_t pageSize;
   std::vector<PageEntry> pageTable;
};

class Memory
{
public:
   ~Memory();

   bool initialise();
   bool alloc(uint32_t address, size_t size);
   bool free(uint32_t address);
   uint32_t allocData(size_t size);

   // Translate WiiU virtual address to host address
   uint8_t *translate(uint32_t address) const
   {
      return mBase + address;
   }

   // Read Type from virtual address
   template<typename Type>
   Type read(uint32_t address) const
   {
      return byte_swap(*reinterpret_cast<Type*>(translate(address)));
   }

   // Read Type from virtual address with no endian byte_swap
   template<typename Type>
   Type readNoSwap(uint32_t address) const
   {
      return *reinterpret_cast<Type*>(translate(address));
   }

   // Write Type to virtual address
   template<typename Type>
   void write(uint32_t address, Type value) const
   {
      *reinterpret_cast<Type*>(translate(address)) = byte_swap(value);
   }

   // Write Type to virtual address with no endian byte_swap
   template<typename Type>
   void writeNoSwap(uint32_t address, Type value) const
   {
      *reinterpret_cast<Type*>(translate(address)) = value;
   }

private:
   MemoryView *getView(uint32_t address);
   bool tryMapViews(uint8_t *base);
   void unmapViews();

   uint8_t *mBase = nullptr;
   HANDLE mFile = NULL;
   std::vector<MemoryView> mViews;
};

extern Memory gMemory;
