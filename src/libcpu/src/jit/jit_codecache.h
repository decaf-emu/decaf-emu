#pragma once
#include "jit_stats.h"

#include <atomic>
#include <cstdint>
#include <common/platform_memory.h>
#include <gsl/gsl>
#include <mutex>

namespace cpu
{

namespace jit
{

/**
 * Code Cache Responsibilities:
 *
 * 1. Map guest address to host address.
 * 2. Allocate executable host memory.
 * 3. Allocate and populate unwind information.
 */
class CodeCache
{
   struct FrameAllocator
   {
      // Memory flags
      platform::ProtectFlags flags;

      //! Base address
      uintptr_t baseAddress;

      //! Amount of data allocated so far.
      std::atomic<size_t> allocated;

      //! Amount of memory committed
      std::atomic<size_t> committed;

      //! Amount of memory reserved
      size_t reserved;

      size_t growthSize;
      std::mutex mutex;
   };

   // Fast Index level sizes
   static constexpr size_t Level1Size = 0x100;
   static constexpr size_t Level2Size = 0x100;
   static constexpr size_t Level3Size = 0x4000;

public:
   CodeCache();
   ~CodeCache();

   bool
   initialise(size_t codeSize,
              size_t dataSize);

   void
   clear();

   void
   free();

   size_t
   getCodeCacheSize();

   size_t
   getDataCacheSize();

   gsl::span<CodeBlock>
   getCompiledCodeBlocks();

   CodeBlock *
   getBlockByAddress(uint32_t address);

   CodeBlock *
   getBlockByIndex(CodeBlockIndex index);

   CodeBlockIndex
   getIndex(uint32_t address);

   CodeBlockIndex
   getIndex(CodeBlock *block);

   std::atomic<CodeBlockIndex> *
   getIndexPointer(uint32_t address);

   void
   setBlockIndex(uint32_t address,
                 CodeBlockIndex index);

   CodeBlock *
   registerCodeBlock(uint32_t address,
                     void *code,
                     size_t size,
                     void *unwindInfo,
                     size_t unwindSize);


private:
   uintptr_t
   allocate(FrameAllocator &allocator,
            size_t size,
            size_t alignment);

private:
   size_t mReserveAddress;
   size_t mReserveSize;
   FrameAllocator mCodeAllocator;
   FrameAllocator mDataAllocator;
   std::atomic<std::atomic<std::atomic<CodeBlockIndex> *> *> *mFastIndex;
};

} // namespace jit

} // namespace cpu
