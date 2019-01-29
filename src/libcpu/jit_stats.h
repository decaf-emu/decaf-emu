#pragma once
#include <array>
#include <atomic>
#include <common/platform.h>
#include <cstdint>
#include <gsl.h>

#ifdef PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace cpu
{

namespace jit
{

#ifdef PLATFORM_WINDOWS
struct CodeBlockUnwindInfo
{
   static constexpr size_t MaxUnwindInfoSize = 4 + 2 * 30 + 8;
   std::array<uint8_t, MaxUnwindInfoSize> data;
   uint32_t size;
   RUNTIME_FUNCTION rtlFuncTable;
};
#else
struct CodeBlockUnwindInfo
{
};
#endif

struct CodeBlockProfileData
{
   std::atomic<uint64_t> count;
   std::atomic<uint64_t> time;
};

struct CodeBlock
{
   //! Guest address of PPC code.
   uint32_t address;

   //! Host address of compiled code.
   void *code;

   //! Size of compiled code.
   uint32_t codeSize;

   //! Profiling data.
   CodeBlockProfileData profileData;

   //! Code block unwind info, only used on Windows.
   CodeBlockUnwindInfo unwindInfo;
};

using CodeBlockIndex = int32_t;

static constexpr CodeBlockIndex CodeBlockIndexUncompiled = -1;
static constexpr CodeBlockIndex CodeBlockIndexCompiling = -2;
static constexpr CodeBlockIndex CodeBlockIndexError = -3;

struct JitStats
{
   uint64_t totalTimeInCodeBlocks = 0;
   uint64_t usedCodeCacheSize = 0;
   uint64_t usedDataCacheSize = 0;
   gsl::span<CodeBlock> compiledBlocks;
};

bool
sampleStats(JitStats &stats);

void
resetProfileStats();

void
setProfilingMask(unsigned mask);

unsigned
getProfilingMask();

} // namespace jit

} // namespace cpu