#include "jit_codecache.h"
#include "jit_stats.h"

#include <atomic>
#include <cstdint>
#include <cstring>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <common/platform.h>
#include <common/platform_memory.h>
#include <gsl/gsl>
#include <mutex>
#include <thread>

namespace cpu
{

namespace jit
{

CodeCache::~CodeCache()
{
   free();
}


/**
 * Initialise the code cache.
 */
bool
CodeCache::initialise(size_t codeSize,
                      size_t dataSize)
{
   mReserveAddress = 0;
   mReserveSize = codeSize + dataSize;

   for (auto n = 2; n < 32; ++n) {
      auto base = 0x100000000 * n;

      if (platform::reserveMemory(base, mReserveSize)) {
         mReserveAddress = base;
         break;
      }
   }

   decaf_assert(mReserveAddress, "Failed to map memory for JIT");

   mCodeAllocator.flags = platform::ProtectFlags::ReadWriteExecute;
   mCodeAllocator.baseAddress = mReserveAddress;
   mCodeAllocator.reserved = codeSize;
   mCodeAllocator.growthSize = 4 * 1024 * 1024;
   mCodeAllocator.committed = 0;
   mCodeAllocator.allocated = 0;

   mDataAllocator.flags = platform::ProtectFlags::ReadWrite;
   mDataAllocator.baseAddress = mReserveAddress + codeSize;
   mDataAllocator.reserved = dataSize;
   mDataAllocator.growthSize = 1 * 1024 * 1024;
   mDataAllocator.committed = 0;
   mDataAllocator.allocated = 0;

   mFastIndex = new std::atomic<std::atomic<CodeBlockIndex> *>[Level1Size];
   std::memset(mFastIndex, 0, sizeof(mFastIndex[0]) * Level1Size);
   return true;
}


/**
 * Clear the code cache.
 *
 * This will also unregister any unwind info with Windows.
 */
void
CodeCache::clear()
{
#ifdef PLATFORM_WINDOWS
   // Delete any registered function tables
   for (auto offset = 0u; offset < mDataAllocator.allocated; offset += sizeof(CodeBlock)) {
      auto blockAddress = mDataAllocator.baseAddress + offset;
      auto block = reinterpret_cast<CodeBlock *>(blockAddress);
      RtlDeleteFunctionTable(&block->unwindInfo.rtlFuncTable);
   }
#endif

   // Reset the allocators, don't bother uncommitting their memory.
   mDataAllocator.allocated = 0;
   mCodeAllocator.allocated = 0;

   // Clear fast index, don't bother unallocating memory.
   if (mFastIndex) {
      for (auto i = 0u; i < Level1Size; ++i) {
         auto level2 = mFastIndex[i].load();

         for (auto j = 0u; level2 && j < Level2Size; ++j) {
            level2[j].store(CodeBlockIndexUncompiled);
         }

         mFastIndex[i].store(nullptr);
      }

      std::memset(mFastIndex, 0, sizeof(mFastIndex[0]) * Level1Size);
   }
}


/**
 * Invalidate a region of code.
 *
 * Because it's super complicated to do properly let's just be a leaky fuck,
 * for now our "invalidation" is really just forgetting that we compiled a block.
 */
void
CodeCache::invalidate(uint32_t base,
                      uint32_t size)
{
   // Find any block containing this address and invalidate them!
   auto blocks = getCompiledCodeBlocks();

   for (auto &block : blocks) {
      auto start = block.address;
      auto end = start + 4096; // FIXME: Just assume 4096 limit for now..

      if (base + size < start) {
         continue;
      }

      if (base >= end) {
         continue;
      }

      getIndexPointer(block.address)->store(CodeBlockIndexUncompiled);
   }
}


/**
 * Free the memory we are using for our JIT.
 */
void
CodeCache::free()
{
   clear();

   if (mFastIndex) {
      for (auto i = 0u; i < Level1Size; ++i) {
         auto level2 = mFastIndex[i].load();

         if (level2) {
            delete[] level2;
         }
      }

      delete[] mFastIndex;
      mFastIndex = nullptr;
   }

   if (mReserveAddress) {
      platform::freeMemory(mReserveAddress, mReserveSize);
      mReserveAddress = 0;
      mReserveSize = 0;
   }
}


/**
 * Returns the amount of data allocated in the code cache.
 */
size_t
CodeCache::getCodeCacheSize()
{
   return mCodeAllocator.allocated;
}


/**
 * Returns the amount of data allocated in the data cache.
 */
size_t
CodeCache::getDataCacheSize()
{
   return mDataAllocator.allocated;
}


/**
 * Returns a list of all compiled code blocks.
 */
gsl::span<CodeBlock>
CodeCache::getCompiledCodeBlocks()
{
   auto count = mDataAllocator.allocated.load() / sizeof(CodeBlock);
   auto first = reinterpret_cast<CodeBlock *>(mDataAllocator.baseAddress);
   return gsl::make_span(first, count);
}


/**
 * Find a compiled code block from it's address.
 */
CodeBlock *
CodeCache::getBlockByAddress(uint32_t address)
{
   auto index = getIndexPointer(address)->load();

   if (index < 0) {
      return nullptr;
   } else {
      return getBlockByIndex(index);
   }
}


/**
 * Find a compiled code block's CodeBlockIndex.
 */
CodeBlockIndex
CodeCache::getIndex(uint32_t address)
{
   return getIndexPointer(address)->load();
}


/**
 * Get a compiled code block's CodeBlockIndex.
 */
CodeBlockIndex
CodeCache::getIndex(CodeBlock *block)
{
   auto blockAddress = reinterpret_cast<uintptr_t>(block);
   auto index = (blockAddress - mDataAllocator.baseAddress) / sizeof(CodeBlock);
   return static_cast<CodeBlockIndex>(index);
}


/**
 * Get a pointer to the CodeBlockIndex for the address.
 *
 * This is used for registering the CodeBlock whilst compiling.
 */
std::atomic<CodeBlockIndex> *
CodeCache::getIndexPointer(uint32_t address)
{
   decaf_check((address & 0x3) == 0);

   auto index1 = (address & 0xFFFF0000) >> 16;
   auto index2 = (address & 0x0000FFFC) >> 2;
   auto level2 = mFastIndex[index1].load();

   if (UNLIKELY(!level2)) {
      auto newLevel2 = new std::atomic<CodeBlockIndex>[Level2Size];
      std::memset(newLevel2, CodeBlockIndexUncompiled, sizeof(newLevel2[0]) * Level2Size);

      if (mFastIndex[index1].compare_exchange_strong(level2, newLevel2)) {
         level2 = newLevel2;
      } else {
         // compare_exchange updates level1 if we were preempted
         delete[] newLevel2;
      }
   }

   return &level2[index2];
}


/**
 * Set a CodeBlockIndex for an address, useful for mirroring duplicate functions.
 */
void
CodeCache::setBlockIndex(uint32_t address,
                         CodeBlockIndex index)
{
   decaf_check(index >= 0);
   getIndexPointer(address)->store(index);
}


/**
 * Register a block of code in the CodeCache.
 *
 * This will allocate memory for the code and data, and update the code block index.
 */
CodeBlock *
CodeCache::registerCodeBlock(uint32_t address,
                             void *code,
                             size_t size,
                             void *unwindInfo,
                             size_t unwindSize)
{
   auto dataAddress = allocate(mDataAllocator, sizeof(CodeBlock), 1);
   auto codeAddress = allocate(mCodeAllocator, size, 16);

   // Setup me block
   auto block = reinterpret_cast<CodeBlock *>(dataAddress);
   block->address = address;
   block->code = reinterpret_cast<void *>(codeAddress);
   block->codeSize = static_cast<uint32_t>(size);
   std::memcpy(block->code, code, size);

   // Initialise profiling data
   block->profileData.count = 0;
   block->profileData.time = 0;

#ifdef PLATFORM_WINDOWS
   // Register unwind info
   decaf_check(unwindSize <= CodeBlockUnwindInfo::MaxUnwindInfoSize);
   block->unwindInfo.size = static_cast<uint32_t>(unwindSize);
   std::memcpy(block->unwindInfo.data.data(), unwindInfo, unwindSize);

   auto unwindAddress = reinterpret_cast<uintptr_t>(block->unwindInfo.data.data());
   block->unwindInfo.rtlFuncTable.BeginAddress = static_cast<DWORD>(codeAddress - mReserveAddress);
   block->unwindInfo.rtlFuncTable.EndAddress = static_cast<DWORD>(block->unwindInfo.rtlFuncTable.BeginAddress + size);
   block->unwindInfo.rtlFuncTable.UnwindData = static_cast<DWORD>(unwindAddress - mReserveAddress);
   RtlAddFunctionTable(&block->unwindInfo.rtlFuncTable, 1, mReserveAddress);
#endif

   auto index = getIndex(block);
   auto indexPtr = getIndexPointer(address);
   indexPtr->store(index);
   return block;
}


/**
 * Allocate memory from the specified CodeCache::FrameAllocator.
 */
uintptr_t
CodeCache::allocate(FrameAllocator &allocator,
                    size_t size,
                    size_t alignment)
{
   auto alignedSize = align_up(size + (alignment - 1), alignment);
   auto offset = allocator.allocated.fetch_add(alignedSize);
   auto alignedOffset = align_up(offset, alignment);

   // Check if we have gone past end of committed memory.
   if (offset + alignedSize > allocator.committed.load()) {
      std::lock_guard<std::mutex> lock { allocator.mutex };
      auto committed = allocator.committed.load();

      while (offset + alignedSize > committed) {
         if (!platform::commitMemory(allocator.baseAddress + committed, allocator.growthSize, allocator.flags)) {
            decaf_abort("Failed to commit memory for JIT");
         }

         committed += allocator.growthSize;
      }

      allocator.committed.store(committed);
   }

   return allocator.baseAddress + offset;
}

} // namespace jit

} // namespace cpu
