#pragma once
#include <cstdint>
#include <vector>
#include "heapmanager.h"

enum class HeapMode : uint8_t
{
   FirstFree,
   NearestSize
};

struct ExpHeapBlock
{
   uint32_t offset;
   uint32_t size;
   uint16_t direction;
   uint16_t groupID;

   bool operator <(const ExpHeapBlock &rhs) const
   {
      return offset < rhs.offset;
   }
};

// TODO: Rewrite to fix alignment issues...
class ExpandedHeapManager : public HeapManager
{
public:
   ExpandedHeapManager(uint32_t address, uint32_t size, HeapFlags flags = HeapFlags::None);
   virtual ~ExpandedHeapManager();

   void free(uint32_t addr);
   uint32_t alloc(uint32_t size, int alignment);
   uint32_t resizeBlock(uint32_t addr, uint32_t size);
   
   uint32_t trimHeap();

   HeapMode getMode();
   uint8_t getGroup();
   uint32_t getLargestFreeBlockSize(int alignment);
   uint32_t getTotalFreeSize();

   HeapMode setMode(HeapMode mode);
   uint8_t setGroup(uint8_t group);

   uint32_t getBlockSize(uint32_t addr);
   uint8_t getBlockGroup(uint32_t addr);
   HeapDirection getBlockDirection(uint32_t addr);

private:
   uint32_t allocNearestSize(HeapDirection direction, uint32_t size);
   uint32_t allocFirst(HeapDirection direction, uint32_t size);

   ExpHeapBlock *findUsedBlockByOffset(uint32_t offset);
   ExpHeapBlock *findUsedBlockByAddress(uint32_t address);

   ExpHeapBlock *findFreeBlockByOffset(uint32_t offset);
   ExpHeapBlock *findFreeBlockByAddress(uint32_t address);
   ExpHeapBlock *findFreeBlockByEndOffset(uint32_t offset);

   void eraseFreeBlockByOffset(uint32_t offset);
   void eraseUsedBlockByOffset(uint32_t offset);

   void addFreeBlock(uint32_t offset, uint32_t size, HeapDirection direction);
   void addUsedBlock(uint32_t offset, uint32_t size, HeapDirection direction);

private:
   HeapMode mMode;
   uint8_t mGroup;
   std::vector<ExpHeapBlock> mFreeBlocks;
   std::vector<ExpHeapBlock> mUsedBlocks;
};
