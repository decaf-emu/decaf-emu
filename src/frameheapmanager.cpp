#include "frameheapmanager.h"
#include "util.h"

#pragma pack(push, 1)

struct FrameHeapState
{
   uint32_t tag;
   uint32_t top;
   uint32_t bottom;
   p32<FrameHeapState> previous;
};

#pragma pack(pop)

FrameHeapManager::FrameHeapManager(uint32_t address, uint32_t size, HeapFlags flags)
   : HeapManager(address, size, flags)
{
   gMemory.alloc(address, size);

   mTop = address + size;
   mBottom = address + sizeof(FrameHeapState);

   mState = make_p32<FrameHeapState>(address);
   mState->tag = 0x11111111;
   mState->top = mTop;
   mState->bottom = mBottom;
   mState->previous = nullptr;
}

FrameHeapManager::~FrameHeapManager()
{
   gMemory.free(mAddress);
}

uint32_t
FrameHeapManager::alloc(uint32_t size, int alignment)
{
   auto direction = HeapDirection::FromBottom;
   auto offset = 0u;

   if (alignment < 0) {
      alignment = -alignment;
      direction = HeapDirection::FromTop;
   }

   // Pad size to alignment
   size = alignUp(size, alignment);

   if (mState->top - mState->bottom < size) {
      // Not enough free space in frame heap
      return 0;
   }

   if (direction == HeapDirection::FromBottom) {
      offset = mState->bottom;
      mState->bottom += size;
   } else if (direction == HeapDirection::FromTop) {
      mState->top -= size;
      offset = mState->top;
   }

   // Bump offset to alignment
   offset = alignUp(offset, alignment);

   return offset;
}

void
FrameHeapManager::free(uint32_t addr, FrameHeapFreeMode::Flags freeMode)
{
   if (freeMode & FrameHeapFreeMode::Tail) {
      if (mState->previous) {
         mState->top = mState->previous->top;
      } else {
         mState->top = mTop;
      }
   }

   if (freeMode & FrameHeapFreeMode::Head) {
      if (mState->previous) {
         mState->bottom = mState->previous->bottom;
      } else {
         mState->bottom = mBottom;
      }
   }
}

uint32_t
FrameHeapManager::trimHeap()
{
   if (mState->top != mTop) {
      // We cannot trim if there has been allocations at the top
      return mSize;
   }

   mSize = mState->bottom - mAddress;
   mTop = mState->bottom;
   mState->top = mTop;
   return mSize;
}

uint32_t
FrameHeapManager::resizeBlock(uint32_t addr, uint32_t newSize)
{
   if (addr > mState->bottom || addr < mBottom) {
      // Invalid address
      return 0;
   }

   auto curSize = mState->bottom - addr;
   auto difSize = newSize - curSize;

   if (mState->top - mState->bottom < difSize) {
      // Not enough free space
      return 0;
   }

   mState->bottom += difSize;
   return newSize;
}

uint32_t
FrameHeapManager::getLargestFreeBlockSize(int alignment)
{
   auto bottom = alignUp(mState->bottom, alignment);
   auto top = alignDown(mState->top, alignment);
   return top - bottom;
}

bool
FrameHeapManager::pushState(uint32_t tag)
{
   auto state = make_p32<FrameHeapState>(alloc(sizeof(FrameHeapState), 4));

   if (!state) {
      return false;
   }

   state->tag = tag;
   state->bottom = mState->bottom;
   state->top = mState->top;
   state->previous = mState;
   mState = state;
   return true;
}

bool
FrameHeapManager::popState(uint32_t findTag)
{
   if (findTag == 0) {
      if (!mState->previous) {
         return false;
      }

      mState = mState->previous;
      return true;
   }

   auto state = mState;

   while (state) {
      if (state->tag == findTag) {
         mState = state;
         return true;
      }

      state = state->previous;
   }

   return false;
}
