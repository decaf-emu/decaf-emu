#pragma once
#include <cstdint>

using WHeapHandle = uint32_t;

enum class HeapFlags : uint32_t
{
   None        = 0,
   Zero        = 1 << 0, // 00 it
   DebugFill   = 1 << 1, // DEADF00D it
   ThreadSafe  = 1 << 2, // Add locks
};

enum class HeapDirection : uint8_t
{
   FromTop,
   FromBottom
};

class HeapManager
{
public:
   HeapManager(uint32_t address, uint32_t size, HeapFlags flags) :
      mAddress(address),
      mSize(size),
      mFlags(flags)
   {
   }

   virtual ~HeapManager()
   {
   }

   uint32_t getAddress()
   {
      return mAddress;
   }

   uint32_t getSize()
   {
      return mSize;
   }

protected:
   uint32_t mAddress;
   uint32_t mSize;
   HeapFlags mFlags;
};
