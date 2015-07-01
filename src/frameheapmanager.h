#pragma once
#include "heapmanager.h"
#include "systemtypes.h"
#include "util.h"

namespace FrameHeapFreeMode
{
enum Flags
{
   Head  = 1 << 0,
   Tail  = 1 << 1,
   All   = Head | Tail
};
}

struct FrameHeapState;

class FrameHeapManager : public HeapManager
{
public:
   FrameHeapManager(uint32_t address, uint32_t size, HeapFlags flags = HeapFlags::None);
   virtual ~FrameHeapManager();

   uint32_t alloc(uint32_t size, int alignment);
   void free(uint32_t addr, FrameHeapFreeMode::Flags freeMode);

   uint32_t trimHeap();
   uint32_t resizeBlock(uint32_t addr, uint32_t newSize);

   bool pushState(uint32_t tag);
   bool popState(uint32_t findTag);

   uint32_t getLargestFreeBlockSize(int alignment);

private:
   uint32_t mTop;
   uint32_t mBottom;
   p32<FrameHeapState> mState;
};
