#pragma once
#include "byte_swap.h"
#include "platform.h"
#include "platform_intrin.h"
#include <vector>

template<typename DataType>
static inline void
byte_swap_unaligned(DataType *dst,
                    const DataType *srcStart,
                    const DataType *srcEnd)
{
   for (auto *src = srcStart; src < srcEnd; ) {
      *dst++ = byte_swap(*src++);
   }
}

#ifdef PLATFORM_HAS_SSE3
template<typename DataType>
static inline void
byte_swap_aligned(DataType *dst,
                  const DataType *srcStart,
                  const DataType *srcEnd)
{
   auto sseDst = reinterpret_cast<__m128i *>(dst);
   auto sseSrc = reinterpret_cast<const __m128i *>(srcStart);
   auto sseSrcEnd = reinterpret_cast<const __m128i *>(srcEnd);

   static_assert(sizeof(DataType) == 2 || sizeof(DataType) == 4,
                 "unexpected data type size for aligned byte swap");

   __m128i sseMask;
   if constexpr (sizeof(DataType) == 2) {
      sseMask = _mm_set_epi8(14, 15, 12, 13, 10, 11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1);
   } else if constexpr (sizeof(DataType) == 4) {
      sseMask = _mm_set_epi8(12, 13, 14, 15, 8, 9, 10, 11, 4, 5, 6, 7, 0, 1, 2, 3);
   }

   while (sseSrc < sseSrcEnd) {
      _mm_storeu_si128(sseDst++,
                       _mm_shuffle_epi8(_mm_loadu_si128(sseSrc++), sseMask));
   }
}
#else
template<typename DataType>
static inline void
byte_swap_aligned(DataType *dst, const DataType *srcStart, const DataType *srcEnd)
{
   byte_swap_unaligned<DataType>(dst, srcStart, srcEnd);
}
#endif

#ifdef PLATFORM_HAS_SSE3
template<typename DataType>
static inline void *
byte_swap_to_scratch(const void *data,
                     uint32_t numBytes,
                     std::vector<uint8_t> &scratch)
{
   // We pad the output buffer to guarentee we can align it to any source address.
   scratch.resize(numBytes + 32);

   // Calculate some information about the indices
   auto swapSrc = reinterpret_cast<const DataType *>(data);
   auto swapSrcEnd = swapSrc + (numBytes / sizeof(DataType));

   // The source must be aligned at least to the swap boundary...
   decaf_check(swapSrc == align_up(swapSrc, sizeof(DataType)));

   // Align our destination exactly the same as the source
   auto unalignedOffset = reinterpret_cast<uintptr_t>(swapSrc) & 0xF;
   auto alignMatchedScratch = align_up(scratch.data(), 16) + unalignedOffset;
   auto swapDest = reinterpret_cast<DataType *>(alignMatchedScratch);

   // Calculate our aligned memory
   auto alignedSwapDest = align_up(swapDest, 16);
   auto alignedSwapSrc = align_up(swapSrc, 16);
   auto alignedSwapSrcEnd = align_down(swapSrcEnd, 16);
   auto alignedSize = alignedSwapSrcEnd - alignedSwapSrc;

   // Do the unaligned before portion
   byte_swap_unaligned<DataType>(swapDest, swapSrc, alignedSwapSrc);

   // Do the aligned portion
   byte_swap_aligned<DataType>(alignedSwapDest, alignedSwapSrc, alignedSwapSrcEnd);

   // Do the unaligned after portion
   byte_swap_unaligned<DataType>(alignedSwapDest + alignedSize,
                                 alignedSwapSrc + alignedSize,
                                 swapSrcEnd);

   return alignMatchedScratch;
}
#else
template<typename DataType>
static inline void *
byte_swap_to_scratch(const void *data, uint32_t numBytes, std::vector<uint8_t>& scratch)
{
   scratch.resize(numBytes);
   auto swapDest = reinterpret_cast<DataType *>(scratch.data());
   auto swapSrc = reinterpret_cast<const DataType *>(data);
   auto swapSrcEnd = swapSrc + (numBytes / sizeof(DataType));
   byte_swap_unaligned<DataType>(swapDest, swapSrc, swapSrcEnd);
   return swapDest;
}
#endif
