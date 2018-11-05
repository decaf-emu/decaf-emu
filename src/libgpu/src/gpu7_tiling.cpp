#include "gpu7_tiling.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <common/align.h>
#include <common/decaf_assert.h>

namespace gpu7::tiling
{

static bool LogMicroShit = false;

static constexpr auto MicroTileWidth = 8;
static constexpr auto MicroTileHeight = 8;
static constexpr auto NumPipes = 2;
static constexpr auto NumBanks = 4;

static constexpr auto PipeInterleaveBytes = 256;
static constexpr auto NumGroupBits = 8;
static constexpr auto NumPipeBits = 1;
static constexpr auto NumBankBits = 2;
static constexpr auto GroupMask = (1 << NumGroupBits) - 1;

static constexpr auto RowSize = 2048;
static constexpr auto SwapSize = 256;
static constexpr auto SplitSize = 2048;
static constexpr auto BankSwapBytes = 256;

static constexpr int MacroTileWidth[] = {
   /* LinearGeneral = */   1,
   /* LinearAligned = */   1,
   /* Tiled1DThin1 = */    1,
   /* Tiled1DThick = */    1,
   /* Tiled2DThin1 = */    NumBanks,
   /* Tiled2DThin2 = */    NumBanks / 2,
   /* Tiled2DThin4 = */    NumBanks / 4,
   /* Tiled2DThick = */    NumBanks,
   /* Tiled2BThin1 = */    NumBanks,
   /* Tiled2BThin2 = */    NumBanks / 2,
   /* Tiled2BThin4 = */    NumBanks / 4,
   /* Tiled2BThick = */    NumBanks,
   /* Tiled3DThin1 = */    NumBanks,
   /* Tiled3DThick = */    NumBanks,
   /* Tiled3BThin1 = */    NumBanks,
   /* Tiled3BThick = */    NumBanks,
};

static constexpr int MacroTileHeight[] = {
   /* LinearGeneral = */   1,
   /* LinearAligned = */   1,
   /* Tiled1DThin1 = */    1,
   /* Tiled1DThick = */    1,
   /* Tiled2DThin1 = */    NumPipes,
   /* Tiled2DThin2 = */    NumPipes * 2,
   /* Tiled2DThin4 = */    NumPipes * 4,
   /* Tiled2DThick = */    NumPipes,
   /* Tiled2BThin1 = */    NumPipes,
   /* Tiled2BThin2 = */    NumPipes * 2,
   /* Tiled2BThin4 = */    NumPipes * 4,
   /* Tiled2BThick = */    NumPipes,
   /* Tiled3DThin1 = */    NumPipes,
   /* Tiled3DThick = */    NumPipes,
   /* Tiled3BThin1 = */    NumPipes,
   /* Tiled3BThick = */    NumPipes,
};

static constexpr int MicroTileThickness[] = {
   /* LinearGeneral = */   1,
   /* LinearAligned = */   1,
   /* Tiled1DThin1 = */    1,
   /* Tiled1DThick = */    4,
   /* Tiled2DThin1 = */    1,
   /* Tiled2DThin2 = */    1,
   /* Tiled2DThin4 = */    1,
   /* Tiled2DThick = */    4,
   /* Tiled2BThin1 = */    1,
   /* Tiled2BThin2 = */    1,
   /* Tiled2BThin4 = */    1,
   /* Tiled2BThick = */    4,
   /* Tiled3DThin1 = */    1,
   /* Tiled3DThick = */    4,
   /* Tiled3BThin1 = */    1,
   /* Tiled3BThick = */    4,
};

static constexpr int
getMacroTileWidth(TileMode tileMode)
{
   return MacroTileWidth[static_cast<size_t>(tileMode)];
}

static constexpr int
getMacroTileHeight(TileMode tileMode)
{
   return MacroTileHeight[static_cast<size_t>(tileMode)];
}

static constexpr int
getMicroTileThickness(TileMode tileMode)
{
   return MicroTileThickness[static_cast<size_t>(tileMode)];
}

struct MicroTiler8
{
   /*
      8 bits per element:
       0:   0,  1,  2,  3,  4,  5,  6,  7,
       8:  16, 17, 18, 19, 20, 21, 22, 23,
      16:   8,  9, 10, 11, 12, 13, 14, 15,
      24:  24, 25, 26, 27, 28, 29, 30, 31,

      32:  32, 33, 34, 35, 36, 37, 38, 39,
      40:  48, 49, 50, 51, 52, 53, 54, 55,
      48:  40, 41, 42, 43, 44, 45, 46, 47,
      56:  56, 57, 58, 59, 60, 61, 62, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      static constexpr auto rowSize = MicroTileWidth * sizeof(uint8_t);
      src += srcOffset;
      dst += dstOffset;

      for (int y = 0; y < MicroTileHeight; y += 4) {
         std::memcpy(dst, src + 0 * srcStrideBytes, rowSize);
         dst += dstStrideBytes;

         std::memcpy(dst, src + 2 * srcStrideBytes, rowSize);
         dst += dstStrideBytes;

         std::memcpy(dst, src + 1 * srcStrideBytes, rowSize);
         dst += dstStrideBytes;

         std::memcpy(dst, src + 3 * srcStrideBytes, rowSize);
         dst += dstStrideBytes;

         src += 4 * srcStrideBytes;
      }
   }
};

struct MicroTiler16
{
   /*
      16 bits per element:
       0:   0,  1,  2,  3,  4,  5,  6,  7,
       8:   8,  9, 10, 11, 12, 13, 14, 15,
      16:  16, 17, 18, 19, 20, 21, 22, 23,
      24:  24, 25, 26, 27, 28, 29, 30, 31,
      32:  32, 33, 34, 35, 36, 37, 38, 39,
      40:  40, 41, 42, 43, 44, 45, 46, 47,
      48:  48, 49, 50, 51, 52, 53, 54, 55,
      56:  56, 57, 58, 59, 60, 61, 62, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      static constexpr auto rowSize = MicroTileWidth * sizeof(uint16_t);
      src += srcOffset;
      dst += dstOffset;

      for (int y = 0; y < MicroTileHeight; ++y) {
         std::memcpy(dst, src, rowSize);
         src += srcStrideBytes;
         dst += dstStrideBytes;
      }
   }
};

struct MicroTiler32
{
   /*
      32 bits per element:
       0:   0,  1,  2,  3,    8,  9, 10, 11,
       8:   4,  5,  6,  7,   12, 13, 14, 15,

      16:  16, 17, 18, 19,   24, 25, 26, 27,
      24:  20, 21, 22, 23,   28, 29, 30, 31,

      32:  32, 33, 34, 35,   40, 41, 42, 43,
      40:  36, 37, 38, 39,   44, 45, 46, 47,

      48:  48, 49, 50, 51,   56, 57, 58, 59,
      56:  52, 53, 54, 55,   60, 61, 62, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      static constexpr auto groupSize = 4 * sizeof(uint32_t);
      src += srcOffset;
      dst += dstOffset;

      for (int y = 0; y < MicroTileHeight; y += 2) {
         auto dstRow1 = reinterpret_cast<uint32_t *>(dst);
         auto dstRow2 = reinterpret_cast<uint32_t *>(dst + dstStrideBytes);

         auto srcRow1 = reinterpret_cast<uint32_t *>(src);
         auto srcRow2 = reinterpret_cast<uint32_t *>(src + srcStrideBytes);

         std::memcpy(dstRow1 + 0, srcRow1 + 0, groupSize);
         std::memcpy(dstRow1 + 4, srcRow2 + 0, groupSize);

         std::memcpy(dstRow2 + 0, srcRow1 + 4, groupSize);
         std::memcpy(dstRow2 + 4, srcRow2 + 4, groupSize);

         src += srcStrideBytes * 2;
         dst += dstStrideBytes * 2;
      }
   }
};

template<bool MacroTiling>
struct MicroTiler64
{
   /*
      64 bits per element:
       0:   0,  1,    4,  5,    8,  9,   12, 13,
       8:   2,  3,    6,  7,   10, 11,   14, 15,

      16:  16, 17,   20, 21,   24, 25,   28, 29,
      24:  18, 19,   22, 23,   26, 27,   30, 31,

      32:  32, 33,   36, 37,   40, 41,   44, 45,
      40:  34, 35,   38, 39,   42, 43,   46, 47,

      48:  48, 49,   52, 53,   56, 57,   60, 61,
      56:  50, 51,   54, 55,   58, 59,   62, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      static constexpr auto groupBytes = 2 * sizeof(uint64_t);
      const auto nextGroupOffset =
         srcOffset + (0x100 << (NumBankBits + NumPipeBits));

      for (int y = 0; y < MicroTileHeight; y += 2) {
         if (MacroTiling && y == 4) {
            // At y == 4 we hit the next group (at element offset 256)
            srcOffset = nextGroupOffset;
         }

         auto dstRow1 = reinterpret_cast<uint64_t *>(dst + dstOffset);
         auto dstRow2 = reinterpret_cast<uint64_t *>(dst + dstOffset + dstStrideBytes);

         auto srcRow1 = reinterpret_cast<uint64_t *>(src + srcOffset);
         auto srcRow2 = reinterpret_cast<uint64_t *>(src + srcOffset + srcStrideBytes);

         std::memcpy(dstRow1 + 0, srcRow1 + 0, groupBytes);
         std::memcpy(dstRow2 + 0, srcRow1 + 2, groupBytes);

         std::memcpy(dstRow1 + 2, srcRow1 + 4, groupBytes);
         std::memcpy(dstRow2 + 2, srcRow1 + 6, groupBytes);

         std::memcpy(dstRow1 + 4, srcRow2 + 0, groupBytes);
         std::memcpy(dstRow2 + 4, srcRow2 + 2, groupBytes);

         std::memcpy(dstRow1 + 6, srcRow2 + 4, groupBytes);
         std::memcpy(dstRow2 + 6, srcRow2 + 6, groupBytes);

         srcOffset += 2 * srcStrideBytes;
         dstOffset += 2 * dstStrideBytes;
      }
   }
};

template<bool MacroTiling>
struct MicroTiler128
{
   /*
      128 bits per element:
         0:   0,  2,    4,  6,    8, 10,   12, 14,
         8:   1,  3,    5,  7,    9, 11,   13, 15,

        16:  16, 18,   20, 22,   24, 26,   28, 30,
        24:  17, 19,   21, 23,   25, 27,   29, 31,

        32:  32, 34,   36, 38,   40, 42,   44, 46,
        40:  33, 35,   37, 39,   41, 43,   45, 47,

        48:  48, 50,   52, 54,   56, 58,   60, 62,
        56:  49, 51,   53, 55,   57, 59,   61, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      static constexpr auto elemBytes = 8;

      for (int y = 0; y < MicroTileHeight; y += 2) {
         auto dstRow1 = reinterpret_cast<uint8_t *>(dst + dstOffset);
         auto dstRow2 = reinterpret_cast<uint8_t *>(dst + dstOffset + dstStrideBytes);

         auto srcRow1 = reinterpret_cast<uint8_t *>(src + srcOffset);
         auto srcRow2 = reinterpret_cast<uint8_t *>(src + srcOffset + srcStrideBytes);

         std::memcpy(dstRow1 + 0 * elemBytes, srcRow1 + 0 * elemBytes, elemBytes);
         std::memcpy(dstRow1 + 1 * elemBytes, srcRow1 + 2 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 0 * elemBytes, srcRow1 + 1 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 1 * elemBytes, srcRow1 + 3 * elemBytes, elemBytes);

         std::memcpy(dstRow1 + 2 * elemBytes, srcRow1 + 4 * elemBytes, elemBytes);
         std::memcpy(dstRow1 + 3 * elemBytes, srcRow1 + 6 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 2 * elemBytes, srcRow1 + 5 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 3 * elemBytes, srcRow1 + 7 * elemBytes, elemBytes);

         std::memcpy(dstRow1 + 4 * elemBytes, srcRow2 + 0 * elemBytes, elemBytes);
         std::memcpy(dstRow1 + 5 * elemBytes, srcRow2 + 2 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 4 * elemBytes, srcRow2 + 1 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 5 * elemBytes, srcRow2 + 3 * elemBytes, elemBytes);

         std::memcpy(dstRow1 + 6 * elemBytes, srcRow2 + 4 * elemBytes, elemBytes);
         std::memcpy(dstRow1 + 7 * elemBytes, srcRow2 + 6 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 6 * elemBytes, srcRow2 + 5 * elemBytes, elemBytes);
         std::memcpy(dstRow2 + 7 * elemBytes, srcRow2 + 7 * elemBytes, elemBytes);

         if (MacroTiling) {
            srcOffset += 0x100 << (NumBankBits + NumPipeBits);
         } else {
            srcOffset += 2 * srcStrideBytes;
         }

         dstOffset += 2 * dstStrideBytes;
      }
   }
};

struct MicroTilerDepth
{
   /*
      depth elements:
          0:   0,  1,  4,  5,   16, 17, 20, 21,
          8:   2,  3,  6,  7,   18, 19, 22, 23,
         16:   8,  9, 12, 13,   24, 25, 28, 29,
         24:  10, 11, 14, 15,   26, 27, 30, 31,

         32:  32, 33, 36, 37,   48, 49, 52, 53,
         40:  34, 35, 38, 39,   50, 51, 54, 55,
         48:  40, 41, 44, 45,   56, 57, 60, 61,
         56:  42, 43, 46, 47,   58, 59, 62, 63,
   */

   static inline void
   apply(const SurfaceInfo &surface,
         uint8_t *src,
         int srcOffset,
         int srcStrideBytes,
         uint8_t *dst,
         int dstOffset,
         int dstStrideBytes)
   {
      decaf_abort("MicroTilerDepth not implemented");
   }
};

template<typename MicroTiler>
static void
applyMicroTiler(const SurfaceInfo &surface,
                void *src,
                void *dst,
                int dstStrideBytes,
                int sliceOffset,
                int microTilesPerRow,
                int microTilesNumRows,
                int microTileBytes)
{
   const auto bytesPerElement = surface.bpp / 8;
   auto microTileOffset = sliceOffset;

   for (auto microTileY = 0; microTileY < microTilesNumRows; ++microTileY) {
      for (auto microTileX = 0; microTileX < microTilesPerRow; ++microTileX) {
         const auto pixelX = microTileX * MicroTileWidth;
         const auto pixelY = microTileY * MicroTileHeight;
         const auto dstOffset =
            pixelX * bytesPerElement + pixelY * dstStrideBytes;

         MicroTiler::apply(surface,
                           static_cast<uint8_t *>(src),
                           microTileOffset,
                           MicroTileWidth * bytesPerElement,
                           static_cast<uint8_t *>(dst),
                           dstOffset,
                           dstStrideBytes);
         microTileOffset += microTileBytes;
      }
   }
}

static void
untileMicroSurface(const SurfaceInfo &surface,
                   void *src,
                   void *dst,
                   int slice,
                   int sample)
{
   const auto bytesPerElement = surface.bpp / 8;

   const auto microTileThickness = getMicroTileThickness(surface.tileMode);
   const auto microTileBytes =
      MicroTileWidth * MicroTileHeight * microTileThickness
      * bytesPerElement * surface.numSamples;

   const auto pitch = calculateAlignedPitch(surface);
   const auto height = calculateAlignedHeight(surface);
   const auto microTilesPerRow = pitch / MicroTileWidth;
   const auto microTilesNumRows = height / MicroTileHeight;

   const auto microTileIndexZ = slice / microTileThickness;
   const  auto sliceBytes =
      pitch * height * microTileThickness * bytesPerElement;
   const auto sliceOffset = microTileIndexZ * sliceBytes;

   const auto dstStrideBytes = pitch * bytesPerElement;

   if (surface.isDepth) {
      // TODO: Implement depth tiling for sample > 0
      decaf_check(sample == 0);
      applyMicroTiler<MicroTilerDepth>(surface, src, dst, dstStrideBytes,
                                       sliceOffset,
                                       microTilesPerRow, microTilesNumRows,
                                       microTileBytes);
      return;
   }

   const auto sampleOffset = sample * (microTileBytes / surface.numSamples);
   const auto sampleSliceOffset = sliceOffset + sampleOffset;

   switch (surface.bpp) {
   case 8:
      applyMicroTiler<MicroTiler8>(surface, src, dst, dstStrideBytes,
                                   sampleSliceOffset,
                                   microTilesPerRow, microTilesNumRows,
                                   microTileBytes);
      break;
   case 16:
      applyMicroTiler<MicroTiler16>(surface, src, dst, dstStrideBytes,
                                    sampleSliceOffset,
                                    microTilesPerRow, microTilesNumRows,
                                    microTileBytes);
      break;
   case 32:
      applyMicroTiler<MicroTiler32>(surface, src, dst, dstStrideBytes,
                                    sampleSliceOffset,
                                    microTilesPerRow, microTilesNumRows,
                                    microTileBytes);
      break;
   case 64:
      applyMicroTiler<MicroTiler64<false>>(surface, src, dst, dstStrideBytes,
                                           sampleSliceOffset,
                                           microTilesPerRow, microTilesNumRows,
                                           microTileBytes);
      break;
   case 128:
      applyMicroTiler<MicroTiler128<false>>(surface, src, dst, dstStrideBytes,
                                            sampleSliceOffset,
                                            microTilesPerRow, microTilesNumRows,
                                            microTileBytes);
      break;
   default:
      decaf_abort("Invalid bpp");
   }
}

template<typename MicroTiler>
static void
applyMacroTiling(const SurfaceInfo &surface,
                 void *src,
                 void *dst,
                 int dstStrideBytes,
                 int sampleOffset,
                 int sliceOffset,
                 int bankSliceRotation,
                 int sampleSliceRotation,
                 int pipeSliceRotation,
                 int bankSwapInterval,
                 int macroTileWidth,
                 int macroTileHeight,
                 int macroTilesPerRow,
                 int macroTilesNumRows,
                 int macroTileBytes,
                 int microTileBytes)
{
   const auto bytesPerElement = surface.bpp / 8;
   auto macroTileOffset = 0;
   auto macroTileIndex = 0;

   for (auto macroTileY = 0; macroTileY < macroTilesNumRows; ++macroTileY) {
      for (auto macroTileX = 0; macroTileX < macroTilesPerRow; ++macroTileX) {
         const auto totalOffset =
            ((sliceOffset + macroTileOffset) >> (NumBankBits + NumPipeBits))
            + sampleOffset;
         const auto offsetHigh =
            (totalOffset & ~GroupMask) << (NumBankBits + NumPipeBits);

         // Our code assumes offsetLow is 0 at this point
         decaf_check((totalOffset & GroupMask) == 0);

         for (auto microTileY = 0; microTileY < macroTileHeight; ++microTileY) {
            for (auto microTileX = 0; microTileX < macroTileWidth; ++microTileX) {
               const  auto pixelX =
                  (macroTileX * macroTileWidth + microTileX) * MicroTileWidth;
               const auto pixelY =
                  (macroTileY * macroTileHeight + microTileY) * MicroTileHeight;
               const auto dstOffset =
                  pixelX * bytesPerElement + pixelY * dstStrideBytes;

               // Calculate bank & pipe
               auto bank = ((pixelX >> 3) & 1) ^ ((pixelY >> 5) & 1);
               bank |= (((pixelX >> 4) & 1) ^ ((pixelY >> 4) & 1)) << 1;
               bank ^= (surface.bankSwizzle + bankSliceRotation) & (NumBanks - 1);
               bank ^= sampleSliceRotation;

               if (bankSwapInterval) {
                  auto bankSwapRotation = (macroTileIndex / bankSwapInterval) % NumBanks;
                  bank ^= bankSwapRotation;
               }

               auto pipe = ((pixelX >> 3) & 1) ^ ((pixelY >> 3) & 1);
               pipe ^= (surface.pipeSwizzle + pipeSliceRotation) & (NumPipes - 1);

               auto microTileOffset =
                  (bank << (NumGroupBits + NumPipeBits))
                  + (pipe << NumGroupBits) + offsetHigh;

               MicroTiler::apply(surface,
                                 static_cast<uint8_t *>(src),
                                 microTileOffset,
                                 MicroTileWidth * bytesPerElement,
                                 static_cast<uint8_t *>(dst),
                                 dstOffset,
                                 dstStrideBytes);
            }
         }

         macroTileOffset += macroTileBytes;
         macroTileIndex++;
      }
   }
}

static void
untileMacroSurface(const SurfaceInfo &surface,
                   void *src,
                   void *dst,
                   int slice,
                   int sample)
{
   const auto bytesPerElement = surface.bpp / 8;

   const auto microTileThickness = getMicroTileThickness(surface.tileMode);
   const auto microTileBytes =
      MicroTileWidth * MicroTileHeight * microTileThickness
      * bytesPerElement * surface.numSamples;

   const auto macroTileWidth = getMacroTileWidth(surface.tileMode);
   const auto macroTileHeight = getMacroTileHeight(surface.tileMode);
   const auto macroTileBytes =
      macroTileWidth * macroTileHeight * microTileBytes;

   const auto pitch = calculateAlignedPitch(surface);
   const auto height = calculateAlignedHeight(surface);
   const auto macroTilesPerRow = pitch / (macroTileWidth * MicroTileWidth);
   const auto macroTilesNumRows = height / (macroTileHeight * MicroTileHeight);
   const auto dstStrideBytes = pitch * bytesPerElement;

   const auto macroTilesPerSlice = macroTilesPerRow * macroTilesNumRows;
   const auto sliceOffset =
      (slice / microTileThickness) * macroTilesPerSlice * macroTileBytes;

   // Depth tiling is different for samples, not yet implemented
   decaf_check(!surface.isDepth || sample == 0);
   const auto sampleOffset = sample * (microTileBytes / surface.numSamples);

   // Calculate bank / pipe rotation
   auto bankSliceRotation = 0;
   auto sampleSliceRotation = 0;
   auto pipeSliceRotation = 0;
   auto bankSwapInterval = 0;

   switch (surface.tileMode) {
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
      bankSliceRotation = ((NumBanks >> 1) - 1) * (slice / microTileThickness);
      sampleSliceRotation = ((NumBanks >> 1) + 1) * sample;
      break;
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
      bankSliceRotation = (slice / microTileThickness) / NumPipes;
      pipeSliceRotation = (slice / microTileThickness);
      break;
   }

   switch (surface.tileMode) {
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
      bankSwapInterval = (bytesPerElement * 8 * 2 / BankSwapBytes);
      break;
   }

   // Do the tiling
   switch (surface.bpp) {
   case 8:
      applyMacroTiling<MicroTiler8>(surface, src, dst, dstStrideBytes,
                                    sampleOffset, sliceOffset,
                                    bankSliceRotation, sampleSliceRotation,
                                    pipeSliceRotation, bankSwapInterval,
                                    macroTileWidth, macroTileHeight,
                                    macroTilesPerRow, macroTilesNumRows,
                                    macroTileBytes, microTileBytes);
      break;
   case 16:
      applyMacroTiling<MicroTiler16>(surface, src, dst, dstStrideBytes,
                                     sampleOffset, sliceOffset,
                                     bankSliceRotation, sampleSliceRotation,
                                     pipeSliceRotation, bankSwapInterval,
                                     macroTileWidth, macroTileHeight,
                                     macroTilesPerRow, macroTilesNumRows,
                                     macroTileBytes, microTileBytes);
      break;
   case 32:
      applyMacroTiling<MicroTiler32>(surface, src, dst, dstStrideBytes,
                                     sampleOffset, sliceOffset,
                                     bankSliceRotation, sampleSliceRotation,
                                     pipeSliceRotation, bankSwapInterval,
                                     macroTileWidth, macroTileHeight,
                                     macroTilesPerRow, macroTilesNumRows,
                                     macroTileBytes, microTileBytes);
      break;
   case 64:
      applyMacroTiling<MicroTiler64<true>>(surface, src, dst, dstStrideBytes,
                                           sampleOffset, sliceOffset,
                                           bankSliceRotation, sampleSliceRotation,
                                           pipeSliceRotation, bankSwapInterval,
                                           macroTileWidth, macroTileHeight,
                                           macroTilesPerRow, macroTilesNumRows,
                                           macroTileBytes, microTileBytes);
      break;
   case 128:
      applyMacroTiling<MicroTiler128<true>>(surface, src, dst, dstStrideBytes,
                                            sampleOffset, sliceOffset,
                                            bankSliceRotation, sampleSliceRotation,
                                            pipeSliceRotation, bankSwapInterval,
                                            macroTileWidth, macroTileHeight,
                                            macroTilesPerRow, macroTilesNumRows,
                                            macroTileBytes, microTileBytes);
      break;
   default:
      decaf_abort("Invalid surface bpp");
   }
}

inline int
NextPow2(int dim)
{
   int newDim = 1;
   while (newDim < dim) {
      newDim <<= 1;
   }
   return newDim;
}

static int
getMipLevelOffset(const SurfaceMipMapInfo &info,
                  int level)
{
   if (level <= 1) {
      return 0;
   }

   return info.offsets[level - 1];
}

static SurfaceInfo
getUnpitchedMipSurfaceInfo(const SurfaceInfo &surface,
                           int level)
{
   const auto macroTileWidth = MicroTileWidth * getMacroTileWidth(surface.tileMode);
   const auto macroTileHeight = MicroTileHeight * getMacroTileHeight(surface.tileMode);
   const auto thickness = getMicroTileThickness(surface.tileMode);

   auto mipSurface = surface;
   mipSurface.width = std::max(1, surface.width >> level);
   mipSurface.height = std::max(1, surface.height >> level);

   if (surface.is3D) {
      mipSurface.depth = std::max(1, surface.depth >> level);
   } else {
      mipSurface.depth = surface.depth;
   }

   if (mipSurface.width < macroTileWidth || mipSurface.height < macroTileHeight) {
      // Once we hit a level where size is smaller than a macro tile, the
      // tile mode becomes micro tiling.
      if (thickness == 4 && !surface.is3D) {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled1DThick;
      } else {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled1DThin1;
      }
   } else if (mipSurface.is3D && mipSurface.depth < 4) {
      // On a 3D texture once we hit depth <4 we siwthc to thin tiling.
      if (mipSurface.tileMode == gpu7::tiling::TileMode::Tiled2DThick) {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled2DThin1;
      } else if (mipSurface.tileMode == gpu7::tiling::TileMode::Tiled2BThick) {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled2BThin1;
      } else if (mipSurface.tileMode == gpu7::tiling::TileMode::Tiled3DThick) {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled3DThin1;
      } else if (mipSurface.tileMode == gpu7::tiling::TileMode::Tiled3BThick) {
         mipSurface.tileMode = gpu7::tiling::TileMode::Tiled3BThin1;
      } else {
         decaf_abort("Unexpected tilemode for 3d texture");
      }
   }

   return mipSurface;
}

static SurfaceInfo
getMipSurfaceInfo(const SurfaceInfo &surface,
                  int level)
{
   auto mipSurface = getUnpitchedMipSurfaceInfo(surface, level);
   mipSurface.depth = NextPow2(mipSurface.depth);
   mipSurface.height = NextPow2(mipSurface.height);
   mipSurface.width = NextPow2(mipSurface.width);
   return mipSurface;
}

static int
calculateHeightAlignment(const SurfaceInfo &surface)
{
   switch (static_cast<TileMode>(surface.tileMode)) {
   case TileMode::Tiled1DThin1:
   case TileMode::Tiled1DThick:
      return MicroTileHeight;
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
      return getMacroTileHeight(surface.tileMode) * MicroTileHeight;
   default:
      return 1;
   }
}

static int
calculatePitchAlignment(const SurfaceInfo &surface)
{
   const auto bytesPerElement = surface.bpp / 8;

   switch (static_cast<TileMode>(surface.tileMode)) {
   case TileMode::LinearAligned:
      return std::max(64, PipeInterleaveBytes / bytesPerElement);
   case TileMode::Tiled1DThin1:
   case TileMode::Tiled1DThick:
      return std::max(MicroTileWidth,
                      PipeInterleaveBytes
                      / (MicroTileHeight * bytesPerElement * surface.numSamples));
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
   {
      auto macroTileWidth = getMacroTileWidth(surface.tileMode);
      auto sampleBytes = bytesPerElement * surface.numSamples;
      return std::max(macroTileWidth * MicroTileWidth,
         ((PipeInterleaveBytes / MicroTileHeight) / sampleBytes) * NumBanks);
   }
   default:
      return 1;
   }
}

int
calculateBaseAddressAlignment(const SurfaceInfo &surface)
{
   switch (static_cast<TileMode>(surface.tileMode)) {
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
   {
      const auto bytesPerElement = surface.bpp / 8;
      const auto microTileThickness = getMicroTileThickness(surface.tileMode);
      const auto microTileBytes =
         MicroTileWidth * MicroTileHeight * microTileThickness
         * bytesPerElement * surface.numSamples;

      const auto macroTileWidth = getMacroTileWidth(surface.tileMode);
      const auto macroTileHeight = getMacroTileHeight(surface.tileMode);
      const auto macroTileBytes =
         macroTileWidth * macroTileHeight * microTileBytes;

      return std::max(macroTileBytes,
                      calculatePitchAlignment(surface) * bytesPerElement
                      * macroTileHeight * MicroTileHeight
                      * surface.numSamples);
   }
   default:
      return PipeInterleaveBytes;
   }
}

int
calculateDepthAlignment(const SurfaceInfo &surface)
{
   switch (static_cast<TileMode>(surface.tileMode)) {
   case TileMode::Tiled1DThin1:
   case TileMode::Tiled1DThick:
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
      return getMicroTileThickness(surface.tileMode);
   default:
      return 1;
   }
}


/**
 * Calculate the tiling aligned depth for the given surface's depth.
 */
int
calculateAlignedDepth(const SurfaceInfo &surface)
{
   return align_up(surface.depth, calculateDepthAlignment(surface));
}


/**
 * Calculate the tiling aligned height for the given surface's height.
 */
int
calculateAlignedHeight(const SurfaceInfo &surface)
{
   return align_up(surface.height, calculateHeightAlignment(surface));
}


/**
 * Calculate the tiling aligned pitch for the given surface's width.
 */
int
calculateAlignedPitch(const SurfaceInfo &surface)
{
   return align_up(surface.width, calculatePitchAlignment(surface));
}


/**
 * Calculates the image size of the given surface.
 *
 * The image means mip level 0, this size does not the include mipmap.
 */
int
calculateImageSize(const SurfaceInfo &surface)
{
   return calculateSliceSize(surface) * calculateAlignedDepth(surface);
}


/**
 * Calculates the size of a single slice of the given surface.
 */
int
calculateSliceSize(const SurfaceInfo &surface)
{
   const auto width = calculateAlignedPitch(surface);
   const auto height = calculateAlignedHeight(surface);
   const auto bytesPerElement = surface.bpp / 8;

   return bytesPerElement * width * height;
}


/**
 * Calculates the size of the given level of a mipmap of the given surface.
 */
int
calculateMipMapLevelSize(const SurfaceInfo &surface,
                         int level)
{
   auto mipSurface = getMipSurfaceInfo(surface, level);
   return calculateImageSize(mipSurface);
}


/**
 * Calculates the size of a single slice of the given level of a mipmap of
 * the given surface.
 */
int
calculateMipMapLevelSliceSize(const SurfaceInfo &surface,
                              int level)
{
   auto mipSurface = getMipSurfaceInfo(surface, level);
   return calculateSliceSize(mipSurface);
}


/**
 * Calculates the image size of the given surface when the extra padding bytes
 * which were required for tiling are removed.
 */
int
calculateUnpitchedImageSize(const SurfaceInfo &surface)
{
   return  calculateUnpitchedSliceSize(surface) * surface.depth;
}


/**
 * Calculates the size of a single slice of the given surface when the extra
 * padding bytes which were required for tiling are removed.
 */
int
calculateUnpitchedSliceSize(const SurfaceInfo &surface)
{
   return (surface.bpp / 8) * surface.width * surface.height;
}


/**
 * Calculates the info of the mipmaps for the given surface when the extra
 * padding bytes which were required for tiling are removed.
 */
void
calculateUnpitchedMipMapInfo(const SurfaceInfo &surface,
                             int numLevels,
                             SurfaceMipMapInfo &info)
{
   const auto bytesPerElem = surface.bpp / 8;
   auto size = 0;

   for (auto level = 1; level < numLevels; ++level) {
      const auto width = std::max(1, surface.width >> level);
      const auto height = std::max(1, surface.height >> level);
      auto depth = surface.depth;

      if (surface.is3D) {
         depth = std::max(1, surface.depth >> level);
      }

      info.offsets[level - 1] = size;
      size += width * height * depth * bytesPerElem;
   }

   info.microTileLevel = -1;
   info.numLevels = numLevels;
   info.size = size;
}


/**
 * Calculates the info of the mipmaps for the given surface when the extra
 * padding bytes which were required for tiling are removed.
 */
void
calculateUnpitchedMipMapSliceInfo(const SurfaceInfo &surface,
                                  int numLevels,
                                  int slice,
                                  SurfaceMipMapInfo &info)
{
   // Copy the surface but change depth to 1 should give us the info for a
   // single slice.
   // TODO: Maybe for future we need to consider slice number for 3D textures
   auto mipMapSurface = surface;
   mipMapSurface.depth = 1;
   calculateUnpitchedMipMapInfo(mipMapSurface, numLevels, info);
}


/**
 * Calculate the mipmap info for the given surface up to numLevels for all
 * slices.
 */
void
calculateMipMapInfo(const SurfaceInfo &surface,
                    int numLevels,
                    SurfaceMipMapInfo &info)
{
   const auto baseAddressAlign = calculateBaseAddressAlignment(surface);
   const auto imageSize = calculateImageSize(surface);

   auto offset = 0;
   auto microTileLevel = 0;

   for (int level = 1; level < numLevels; ++level) {
      const auto mipSurface = getMipSurfaceInfo(surface, level);
      offset = align_up(offset, baseAddressAlign);

      if (level == 1) {
         info.offsets[0] = align_up(imageSize, baseAddressAlign);
      } else {
         info.offsets[level - 1] = offset;
      }

      offset += calculateImageSize(mipSurface);
   }

   info.numLevels = numLevels;
   info.microTileLevel = microTileLevel;
   info.size = offset;
}

void
calculateMipMapSliceInfo(const SurfaceInfo &surface,
                         int numLevels,
                         int /* slice */,
                         SurfaceMipMapInfo &info)
{
   // Copy the surface but change depth to 1 should give us the info for a
   // single slice.
   // TODO: Maybe for future we need to consider slice number for 3D textures
   auto mipMapSurface = surface;
   mipMapSurface.depth = 1;
   calculateMipMapInfo(mipMapSurface, numLevels, info);
}

void
untileImage(const SurfaceInfo &surface,
            void *src,
            void *dst)
{
   const auto sliceSize = calculateSliceSize(surface);

   // Multi-sample untile is not supported yet
   decaf_check(surface.numSamples == 1);

   for (auto slice = 0; slice < surface.depth; ++slice) {
      untileSlice(surface, src,
                  reinterpret_cast<uint8_t *>(dst) + sliceSize * slice,
                  slice, 0);
   }
}

void
untileSlice(const SurfaceInfo &surface,
            void *src,
            void *dst,
            int slice,
            int sample)
{
   // Sample decoding is not supported yet.
   decaf_check(sample == 0);

   switch (static_cast<TileMode>(surface.tileMode)) {
   case TileMode::LinearGeneral:
   case TileMode::LinearAligned:
      // Already "untiled"
      return;
   case TileMode::Tiled1DThin1:
      return untileMicroSurface(surface, src, dst, slice, sample);
   case TileMode::Tiled1DThick:
      return untileMicroSurface(surface, src, dst, slice, sample);
   case TileMode::Tiled2DThin1:
   case TileMode::Tiled2DThin2:
   case TileMode::Tiled2DThin4:
   case TileMode::Tiled2DThick:
   case TileMode::Tiled2BThin1:
   case TileMode::Tiled2BThin2:
   case TileMode::Tiled2BThin4:
   case TileMode::Tiled2BThick:
   case TileMode::Tiled3DThin1:
   case TileMode::Tiled3DThick:
   case TileMode::Tiled3BThin1:
   case TileMode::Tiled3BThick:
      return untileMacroSurface(surface, src, dst, slice, sample);
   default:
      decaf_abort("Invalid tile mode");
   }
}

void
untileMipMaps(const SurfaceInfo &surface,
              const SurfaceMipMapInfo &mipMapInfo,
              void *src,
              void *dst)
{
   for (auto level = 1; level < mipMapInfo.numLevels; ++level) {
      const auto mipSurface = getMipSurfaceInfo(surface, level);
      const auto offset = getMipLevelOffset(mipMapInfo, level);
      const auto sliceSize = calculateSliceSize(mipSurface);

      const auto mipSrc = reinterpret_cast<uint8_t *>(src) + offset;
      const auto mipDst = reinterpret_cast<uint8_t *>(dst) + offset;

      for (auto slice = 0; slice < mipSurface.depth; ++slice) {
         untileSlice(mipSurface,
                     mipSrc,
                     mipDst + sliceSize * slice, slice, 0);
      }
   }
}

void
untileMipMapsForSlice(const SurfaceInfo &surface,
                      const SurfaceMipMapInfo &mipMapSliceInfo,
                      const SurfaceMipMapInfo &mipMapUnpitchedSliceInfo,
                      void *src,
                      void *dst,
                      int slice,
                      int sample)
{
   for (auto level = 1; level < mipMapSliceInfo.numLevels; ++level) {
      const auto mipSurface = getMipSurfaceInfo(surface, level);
      const auto srcOffset = getMipLevelOffset(mipMapSliceInfo, level);
      const auto dstOffset = getMipLevelOffset(mipMapUnpitchedSliceInfo, level);

      untileSlice(mipSurface,
                  reinterpret_cast<uint8_t *>(src) + srcOffset,
                  reinterpret_cast<uint8_t *>(dst) + dstOffset,
                  slice,
                  0);
   }
}

void
unpitchSlice(const SurfaceInfo &surface,
             void *src,
             void *dst)
{
   const auto bytesPerElement = surface.bpp / 8;
   const auto srcPitch = calculateAlignedPitch(surface);
   const auto srcStride = srcPitch * bytesPerElement;
   const auto dstStride = surface.width * bytesPerElement;

   for (auto y = 0; y < surface.height; ++y) {
      std::memcpy(reinterpret_cast<uint8_t *>(dst) + y * dstStride,
                  reinterpret_cast<uint8_t *>(src) + y * srcStride,
                  dstStride);
   }
}

void
unpitchImage(const SurfaceInfo &surface,
             void *src,
             void *dst)
{
   const auto srcSliceSize = calculateSliceSize(surface);
   const auto dstSliceSize = calculateUnpitchedSliceSize(surface);

   for (auto slice = 0; slice < surface.depth; ++slice) {
      auto sliceSrc = reinterpret_cast<uint8_t *>(src) + srcSliceSize * slice;
      auto sliceDst = reinterpret_cast<uint8_t *>(dst) + dstSliceSize * slice;
      unpitchSlice(surface, sliceSrc, sliceDst);
   }
}

void
unpitchMipMaps(const SurfaceInfo &surface,
               const SurfaceMipMapInfo &untiledMipMapInfo,
               const SurfaceMipMapInfo &unpitchedMipMapInfo,
               void *src,
               void *dst)
{
   for (auto level = 1; level < untiledMipMapInfo.numLevels; ++level) {
      const auto srcOffset = getMipLevelOffset(untiledMipMapInfo, level);
      const auto dstOffset = getMipLevelOffset(unpitchedMipMapInfo, level);

      unpitchImage(getUnpitchedMipSurfaceInfo(surface, level),
                   reinterpret_cast<uint8_t *>(src) + srcOffset,
                   reinterpret_cast<uint8_t *>(dst) + dstOffset);
   }
}

void
unpitchMipMapsForSlice(const SurfaceInfo &surface,
                       const SurfaceMipMapInfo &untiledMipMapInfo,
                       const SurfaceMipMapInfo &unpitchedMipMapInfo,
                       void *src,
                       void *dst,
                       int slice)
{
   for (auto level = 1; level < untiledMipMapInfo.numLevels; ++level) {
      const auto srcOffset = getMipLevelOffset(untiledMipMapInfo, level);
      const auto dstOffset = getMipLevelOffset(unpitchedMipMapInfo, level);

      unpitchSlice(getUnpitchedMipSurfaceInfo(surface, level),
                   reinterpret_cast<uint8_t *>(src) + srcOffset,
                   reinterpret_cast<uint8_t *>(dst) + dstOffset);
   }
}

} // namespace gpu7::tiling
