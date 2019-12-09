#include "gpu7_tiling_cpu.h"

#include <common/align.h>
#include <common/decaf_assert.h>

#include <cstring>

namespace gpu7::tiling::cpu
{

template<
   bool IsUntiling,
   uint32_t MicroTileThickness,
   uint32_t MacroTileWidth,
   uint32_t MacroTileHeight,
   bool IsMacro3X,
   bool IsBankSwapped,
   uint32_t BitsPerElement,
   bool IsDepth
>
struct RetileCore
{
   static constexpr bool IsMacroTiling = (MacroTileWidth > 1 || MacroTileHeight > 1);
   static constexpr uint32_t BytesPerElement = BitsPerElement / 8;
   static constexpr uint32_t MicroTileBytes = MicroTileWidth * MicroTileHeight * MicroTileThickness * BytesPerElement;
   static constexpr uint32_t MacroTileBytes = MacroTileWidth * MacroTileHeight * MicroTileBytes;

   template<uint32_t bytesPerElem>
   static inline void
   copyElems(uint8_t *untiled, uint8_t *tiled, size_t numElems)
   {
      if (IsUntiling) {
         std::memcpy(untiled, tiled, bytesPerElem * numElems);
      } else {
         std::memcpy(tiled, untiled, bytesPerElem * numElems);
      }
   }

   static inline void
   retileMicro8(uint8_t *tiled,
                uint8_t *untiled,
                uint32_t untiledStride)
   {
      static constexpr auto tiledStride = MicroTileWidth;
      static constexpr auto rowElems = MicroTileWidth / 8;

      for (int y = 0; y < MicroTileHeight; y += 4) {
         auto untiledRow0 = untiled + 0 * untiledStride;
         auto untiledRow1 = untiled + 1 * untiledStride;
         auto untiledRow2 = untiled + 2 * untiledStride;
         auto untiledRow3 = untiled + 3 * untiledStride;

         auto tiledRow0 = tiled + 0 * tiledStride;
         auto tiledRow1 = tiled + 1 * tiledStride;
         auto tiledRow2 = tiled + 2 * tiledStride;
         auto tiledRow3 = tiled + 3 * tiledStride;

         copyElems<8>(untiledRow0, tiledRow0, rowElems);
         copyElems<8>(untiledRow1, tiledRow2, rowElems);
         copyElems<8>(untiledRow2, tiledRow1, rowElems);
         copyElems<8>(untiledRow3, tiledRow3, rowElems);

         untiled += 4 * untiledStride;
         tiled += 4 * tiledStride;
      }
   }

   static inline void
   retileMicro16(uint8_t *tiled,
                 uint8_t *untiled,
                 uint32_t untiledStride)
   {
      static constexpr auto tiledStride = MicroTileWidth * 2;
      static constexpr auto rowElems = MicroTileWidth * 2 / 16;

      for (int y = 0; y < MicroTileHeight; ++y) {
         copyElems<16>(untiled, tiled, rowElems);

         untiled += untiledStride;
         tiled += tiledStride;
      }
   }

   static inline void
   retileMicro32(uint8_t *tiled,
                 uint8_t *untiled,
                 uint32_t untiledStride)
   {
      static constexpr auto tiledStride = MicroTileWidth * 4;
      static constexpr auto groupElems = 4 * 4 / 16;

      for (int y = 0; y < MicroTileHeight; y += 2) {
         auto untiledRow1 = untiled + 0 * untiledStride;
         auto untiledRow2 = untiled + 1 * untiledStride;

         auto tiledRow1 = tiled + 0 * tiledStride;
         auto tiledRow2 = tiled + 1 * tiledStride;

         copyElems<16>(untiledRow1 + 0, tiledRow1 + 0, groupElems);
         copyElems<16>(untiledRow1 + 16, tiledRow2 + 0, groupElems);

         copyElems<16>(untiledRow2 + 0, tiledRow1 + 16, groupElems);
         copyElems<16>(untiledRow2 + 16, tiledRow2 + 16, groupElems);

         tiled += tiledStride * 2;
         untiled += untiledStride * 2;
      }
   }

   static inline void
   retileMicro64(uint8_t *tiled,
                 uint8_t *untiled,
                 uint32_t untiledStride)
   {
      static constexpr auto tiledStride = MicroTileWidth * 8;
      static constexpr auto groupElems = 2 * (64 / 8) / 16;

      for (int y = 0; y < MicroTileHeight; y += 2) {
         if constexpr (IsMacroTiling) {
            if (y == 4) {
               // At y == 4 we hit the next group (at element offset 256)
               tiled -= tiledStride * y;
               tiled += 0x100 << (NumBankBits + NumPipeBits);
            }
         }

         auto untiledRow1 = untiled + 0 * untiledStride;
         auto untiledRow2 = untiled + 1 * untiledStride;

         auto tiledRow1 = tiled + 0 * tiledStride;
         auto tiledRow2 = tiled + 1 * tiledStride;

         copyElems<16>(untiledRow1 + 0, tiledRow1 + 0, groupElems);
         copyElems<16>(untiledRow2 + 0, tiledRow1 + 16, groupElems);

         copyElems<16>(untiledRow1 + 16, tiledRow1 + 32, groupElems);
         copyElems<16>(untiledRow2 + 16, tiledRow1 + 48, groupElems);

         copyElems<16>(untiledRow1 + 32, tiledRow2 + 0, groupElems);
         copyElems<16>(untiledRow2 + 32, tiledRow2 + 16, groupElems);

         copyElems<16>(untiledRow1 + 48, tiledRow2 + 32, groupElems);
         copyElems<16>(untiledRow2 + 48, tiledRow2 + 48, groupElems);

         tiled += tiledStride * 2;
         untiled += untiledStride * 2;
      }
   }

   static inline void
   retileMicro128(uint8_t *tiled,
                  uint8_t *untiled,
                  uint32_t untiledStride)
   {
      static constexpr auto tiledStride = MicroTileWidth * 16;
      static constexpr auto groupBytes = 16;
      static constexpr auto groupElems = 16 / 16;

      for (int y = 0; y < MicroTileHeight; y += 2) {
         auto untiledRow1 = untiled + 0 * untiledStride;
         auto untiledRow2 = untiled + 1 * untiledStride;

         auto tiledRow1 = tiled + 0 * tiledStride;
         auto tiledRow2 = tiled + 1 * tiledStride;

         copyElems<16>(untiledRow1 + 0 * groupBytes, tiledRow1 + 0 * groupBytes, groupElems);
         copyElems<16>(untiledRow1 + 1 * groupBytes, tiledRow1 + 2 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 0 * groupBytes, tiledRow1 + 1 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 1 * groupBytes, tiledRow1 + 3 * groupBytes, groupElems);

         copyElems<16>(untiledRow1 + 2 * groupBytes, tiledRow1 + 4 * groupBytes, groupElems);
         copyElems<16>(untiledRow1 + 3 * groupBytes, tiledRow1 + 6 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 2 * groupBytes, tiledRow1 + 5 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 3 * groupBytes, tiledRow1 + 7 * groupBytes, groupElems);

         copyElems<16>(untiledRow1 + 4 * groupBytes, tiledRow2 + 0 * groupBytes, groupElems);
         copyElems<16>(untiledRow1 + 5 * groupBytes, tiledRow2 + 2 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 4 * groupBytes, tiledRow2 + 1 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 5 * groupBytes, tiledRow2 + 3 * groupBytes, groupElems);

         copyElems<16>(untiledRow1 + 6 * groupBytes, tiledRow2 + 4 * groupBytes, groupElems);
         copyElems<16>(untiledRow1 + 7 * groupBytes, tiledRow2 + 6 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 6 * groupBytes, tiledRow2 + 5 * groupBytes, groupElems);
         copyElems<16>(untiledRow2 + 7 * groupBytes, tiledRow2 + 7 * groupBytes, groupElems);

         if (IsMacroTiling) {
            tiled += 0x100 << (NumBankBits + NumPipeBits);
         } else {
            tiled += tiledStride * 2;
         }

         untiled += untiledStride * 2;
      }
   }

   static inline void
   copyDepthXYGroup(uint8_t *tiled,
                    uint8_t *untiled,
                    uint32_t untiledStride,
                    uint32_t tX, uint32_t tY,
                    uint32_t uX, uint32_t uY)
   {
      static constexpr auto groupBytes = 2 * BytesPerElement;
      static constexpr auto groupElems = 2 * BytesPerElement / 4;
      static constexpr auto tiledStride = MicroTileWidth * BytesPerElement;

      copyElems<4>(untiled + uY * untiledStride + uX * groupBytes, tiled + tY * tiledStride + tX * groupBytes, groupElems);
   }

   static inline void
   retileMicroDepth(uint8_t *tiled,
                    uint8_t *untiled,
                    uint32_t untiledStride)
   {
      for (int y = 0; y < MicroTileHeight; y += 4) {
         copyDepthXYGroup(tiled, untiled, untiledStride, 0, y + 0, 0, y + 0);
         copyDepthXYGroup(tiled, untiled, untiledStride, 1, y + 0, 0, y + 1);
         copyDepthXYGroup(tiled, untiled, untiledStride, 2, y + 0, 1, y + 0);
         copyDepthXYGroup(tiled, untiled, untiledStride, 3, y + 0, 1, y + 1);

         copyDepthXYGroup(tiled, untiled, untiledStride, 0, y + 1, 0, y + 2);
         copyDepthXYGroup(tiled, untiled, untiledStride, 1, y + 1, 0, y + 3);
         copyDepthXYGroup(tiled, untiled, untiledStride, 2, y + 1, 1, y + 2);
         copyDepthXYGroup(tiled, untiled, untiledStride, 3, y + 1, 1, y + 3);

         copyDepthXYGroup(tiled, untiled, untiledStride, 0, y + 2, 2, y + 0);
         copyDepthXYGroup(tiled, untiled, untiledStride, 1, y + 2, 2, y + 1);
         copyDepthXYGroup(tiled, untiled, untiledStride, 2, y + 2, 3, y + 0);
         copyDepthXYGroup(tiled, untiled, untiledStride, 3, y + 2, 3, y + 1);

         copyDepthXYGroup(tiled, untiled, untiledStride, 0, y + 3, 2, y + 2);
         copyDepthXYGroup(tiled, untiled, untiledStride, 1, y + 3, 2, y + 3);
         copyDepthXYGroup(tiled, untiled, untiledStride, 2, y + 3, 3, y + 2);
         copyDepthXYGroup(tiled, untiled, untiledStride, 3, y + 3, 3, y + 3);
      }
   }

   struct Params
   {
      uint32_t firstSliceIndex;

      // Micro tiling parameters
      uint32_t numTilesPerRow;
      uint32_t numTilesPerSlice;
      uint32_t thinMicroTileBytes;
      uint32_t thickSliceBytes;

      // Macro tiling parameters
      uint32_t bankSwizzle;
      uint32_t pipeSwizzle;
      uint32_t bankSwapWidth;
   };

   /*
   We always execute in a problem-space which starts at a thick-slice
   boundary.  The tiled pointer will point to the start of that boundary
   and the untiled pointer will point to the start of a specific slice
   within it (if untiling on a not-thick-slice-aligned boundary).
   */

   static inline void
   retileMicro(const Params& params,
               uint32_t tileIndex,
               uint8_t *untiled,
               uint8_t *tiled)
   {
      const uint32_t thinSliceBytes = params.thickSliceBytes / MicroTileThickness;
      const uint32_t untiledStride = params.numTilesPerRow * MicroTileWidth * BytesPerElement;
      const uint32_t thickMicroTileBytes = params.thinMicroTileBytes * MicroTileThickness;

      const uint32_t dispatchSliceIndex = tileIndex / params.numTilesPerSlice;
      const uint32_t sliceTileIndex = tileIndex % params.numTilesPerSlice;

      // Find the global slice index we are currently at.
      const uint32_t srcSliceIndex = params.firstSliceIndex + dispatchSliceIndex;

      // We need to identify where inside the current thick slice we are.
      const uint32_t localSliceIndex = srcSliceIndex % MicroTileThickness;

      // Calculate the offset to our untiled data starting from the thick slice
      const uint32_t srcTileY = sliceTileIndex / params.numTilesPerRow;
      const uint32_t srcTileX = sliceTileIndex % params.numTilesPerRow;

      uint32_t untiledOffset =
         (localSliceIndex * thinSliceBytes) +
         (srcTileX * MicroTileWidth * BytesPerElement) +
         (srcTileY * params.numTilesPerRow * params.thinMicroTileBytes);

      // Calculate the offset to our tiled data starting from the thick slice
      uint32_t tiledOffset =
         (localSliceIndex * params.thinMicroTileBytes) +
         sliceTileIndex * thickMicroTileBytes;

      // In the case that we are using thick micro tiles, we need to advance our
      // offsets to the current thick slice boundary that we are at.
      const uint32_t firstThickSliceIndex = params.firstSliceIndex / MicroTileThickness;
      const uint32_t thickSliceIndex = srcSliceIndex / MicroTileThickness;
      const uint32_t thickSliceOffset = (thickSliceIndex - firstThickSliceIndex) * params.thickSliceBytes;
      tiledOffset += thickSliceOffset;
      untiledOffset += thickSliceOffset;

      // The untiled pointers are offset forward by the local slice index already,
      // we need to back it up since our calculations above consider it.
      const uint32_t firstThinSliceIndex = params.firstSliceIndex % MicroTileThickness;
      untiledOffset -= firstThinSliceIndex * thinSliceBytes;

      // Update our pointers based on the calculated offset.
      tiled = tiled + tiledOffset;
      untiled = untiled + untiledOffset;

      if constexpr (IsDepth) {
         retileMicroDepth(tiled, untiled, untiledStride);
      } else {
         if constexpr (BitsPerElement == 8) {
            retileMicro8(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 16) {
            retileMicro16(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 32) {
            retileMicro32(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 64) {
            retileMicro64(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 128) {
            retileMicro128(tiled, untiled, untiledStride);
         }
      }
   }

   static inline void
   retileMacro(const Params& params,
               uint32_t tileIndex,
               uint8_t *untiled,
               uint8_t *tiled)
   {
      const uint32_t thinSliceBytes = params.thickSliceBytes / MicroTileThickness;
      const uint32_t untiledStride = params.numTilesPerRow * MicroTileWidth * BytesPerElement;

      const uint32_t dispatchSliceIndex = tileIndex / params.numTilesPerSlice;
      const uint32_t sliceTileIndex = tileIndex % params.numTilesPerSlice;

      // Find the global slice index we are currently at.
      const uint32_t srcSliceIndex = params.firstSliceIndex + dispatchSliceIndex;

      // We need to identify where inside the current thick slice we are.
      const uint32_t localSliceIndex = srcSliceIndex % MicroTileThickness;

      // Calculate the thickSliceIndex
      const uint32_t thickSliceIndex = srcSliceIndex / MicroTileThickness;

      // Calculate our tile positions
      const uint32_t microTilesPerMacro = MacroTileWidth * MacroTileHeight;
      const uint32_t macroTilesPerRow = params.numTilesPerRow / MacroTileWidth;
      const uint32_t microTilesPerMacroRow = microTilesPerMacro * macroTilesPerRow;

      const uint32_t srcMacroTileY = sliceTileIndex / microTilesPerMacroRow;
      const uint32_t macroRowTileIndex = sliceTileIndex % microTilesPerMacroRow;

      const uint32_t srcMacroTileX = macroRowTileIndex / microTilesPerMacro;
      const uint32_t microTileIndex = macroRowTileIndex % microTilesPerMacro;

      const uint32_t srcMicroTileY = microTileIndex / MacroTileWidth;
      const uint32_t srcMicroTileX = microTileIndex % MacroTileWidth;

      const uint32_t srcTileX = srcMacroTileX * MacroTileWidth + srcMicroTileX;
      const uint32_t srcTileY = srcMacroTileY * MacroTileHeight + srcMicroTileY;

      // Figure out what our untiled offset shall be
      uint32_t untiledOffset =
         (localSliceIndex * thinSliceBytes) +
         (srcTileX * MicroTileWidth * BytesPerElement) +
         (srcTileY * MicroTileHeight * untiledStride);

      // Calculate the offset to our untiled data starting from the thick slice
      const uint32_t macroTileIndex = (srcMacroTileY * macroTilesPerRow) + srcMacroTileX;
      const uint32_t macroTileOffset = macroTileIndex * MacroTileBytes;
      const uint32_t tiledBaseOffset =
         (macroTileOffset >> (NumBankBits + NumPipeBits)) +
         (localSliceIndex * params.thinMicroTileBytes);

      const uint32_t offsetHigh = (tiledBaseOffset & ~GroupMask) << (NumBankBits + NumPipeBits);
      const uint32_t offsetLow = tiledBaseOffset & GroupMask;

      // Calculate our bank/pipe/sample rotations and swaps
      uint32_t bankSliceRotation = 0;
      uint32_t pipeSliceRotation = 0;
      if (!IsMacro3X) {
         // 2_ format
         bankSliceRotation = ((NumBanks >> 1) - 1)* thickSliceIndex;
      } else {
         // 3_ format
         bankSliceRotation = thickSliceIndex / NumPipes;
         pipeSliceRotation = thickSliceIndex;
      }

      uint32_t bankSwapRotation = 0;
      if (IsBankSwapped) {
         const uint32_t bankSwapOrder[] = { 0, 1, 3, 2 };
         const uint32_t swapIndex = ((srcMacroTileX * MicroTileWidth * MacroTileWidth) / params.bankSwapWidth);
         bankSwapRotation = bankSwapOrder[swapIndex % NumBanks];
      }

      uint32_t bank = 0;
      bank |= ((srcTileX >> 0) & 1) ^ ((srcTileY >> 2) & 1);
      bank |= (((srcTileX >> 1) & 1) ^ ((srcTileY >> 1) & 1)) << 1;
      bank ^= (params.bankSwizzle + bankSliceRotation) & (NumBanks - 1);
      bank ^= bankSwapRotation;

      uint32_t pipe = 0;
      pipe |= ((srcTileX >> 0) & 1) ^ ((srcTileY >> 0) & 1);
      pipe ^= (params.pipeSwizzle + pipeSliceRotation) & (NumPipes - 1);

      uint32_t tiledOffset =
         (bank << (NumGroupBits + NumPipeBits)) |
         (pipe << NumGroupBits) |
         offsetLow | offsetHigh;

      // In the case that we are using thick micro tiles, we need to advance our
      // offsets to the current thick slice boundary that we are at.
      const uint32_t firstThickSliceIndex = params.firstSliceIndex / MicroTileThickness;
      const uint32_t thickSliceOffset = (thickSliceIndex - firstThickSliceIndex) * params.thickSliceBytes;
      tiledOffset += thickSliceOffset;
      untiledOffset += thickSliceOffset;

      // The untiled pointers are offset forward by the local slice index already,
      // we need to back it up since our calculations above consider it.
      const uint32_t firstThinSliceIndex = params.firstSliceIndex % MicroTileThickness;
      untiledOffset -= firstThinSliceIndex * thinSliceBytes;

      // Update our pointers based on the calculated offset.
      tiled = tiled + tiledOffset;
      untiled = untiled + untiledOffset;

      if constexpr (IsDepth) {
         retileMicroDepth(tiled, untiled, untiledStride);
      } else {
         if constexpr (BitsPerElement == 8) {
            retileMicro8(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 16) {
            retileMicro16(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 32) {
            retileMicro32(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 64) {
            retileMicro64(tiled, untiled, untiledStride);
         } else if constexpr (BitsPerElement == 128) {
            retileMicro128(tiled, untiled, untiledStride);
         }
      }
   }

   static inline void
   retile(const Params& params,
          uint32_t tileIndex,
          uint8_t *untiled,
          uint8_t *tiled)
   {
      if constexpr (IsMacroTiling) {
         retileMacro(params, tileIndex, untiled, tiled);
      } else {
         retileMicro(params, tileIndex, untiled, tiled);
      }
   }
};

template<bool IsUntiling,
   TileMode RetileMode,
   uint32_t BitsPerElement,
   bool IsDepth>
   static inline void
retileTiledSurface3(const RetileInfo& info,
                    uint8_t *untiled,
                    uint8_t *tiled,
                    uint32_t firstSlice,
                    uint32_t numSlices)
{
   using Retiler = RetileCore<
      IsUntiling,
      getMicroTileThickness(RetileMode),
      getMacroTileWidth(RetileMode),
      getMacroTileHeight(RetileMode),
      getTileModeIs3X(RetileMode),
      getTileModeIsBankSwapped(RetileMode),
      BitsPerElement,
      IsDepth>;

   typename Retiler::Params params;
   params.firstSliceIndex = firstSlice;

   params.numTilesPerRow = info.numTilesPerRow;
   params.numTilesPerSlice = info.numTilesPerSlice;
   params.thinMicroTileBytes = info.thickMicroTileBytes / info.microTileThickness;
   params.thickSliceBytes = info.thinSliceBytes * info.microTileThickness;

   params.bankSwizzle = info.bankSwizzle;
   params.pipeSwizzle = info.pipeSwizzle;
   params.bankSwapWidth = info.bankSwapWidth;

   uint32_t numTiles = numSlices * info.numTilesPerSlice;
   for (auto tileIndex = 0u; tileIndex < numTiles; ++tileIndex) {
      Retiler::retile(params, tileIndex, untiled, tiled);
   }
}

template<bool IsUntiling, TileMode RetileMode>
static inline void
retileTiledSurface2(const RetileInfo& info,
                    uint8_t *untiled,
                    uint8_t *tiled,
                    uint32_t firstSlice,
                    uint32_t numSlices)
{
   if (!info.isDepth) {
      if (info.bitsPerElement == 8) {
         retileTiledSurface3<IsUntiling, RetileMode, 8, false>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 16) {
         retileTiledSurface3<IsUntiling, RetileMode, 16, false>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 32) {
         retileTiledSurface3<IsUntiling, RetileMode, 32, false>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 64) {
         retileTiledSurface3<IsUntiling, RetileMode, 64, false>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 128) {
         retileTiledSurface3<IsUntiling, RetileMode, 128, false>(info, untiled, tiled, firstSlice, numSlices);
      } else {
         decaf_abort("Invalid color surface bpp");
      }
   } else {
      if (info.bitsPerElement == 16) {
         retileTiledSurface3<IsUntiling, RetileMode, 16, true>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 32) {
         retileTiledSurface3<IsUntiling, RetileMode, 32, true>(info, untiled, tiled, firstSlice, numSlices);
      } else if (info.bitsPerElement == 64) {
         retileTiledSurface3<IsUntiling, RetileMode, 64, true>(info, untiled, tiled, firstSlice, numSlices);
      } else {
         decaf_abort("Invalid depth surface bpp");
      }
   }
}

template<bool IsUntiling>
static inline void
retileTiledSurface(const RetileInfo& info,
                   uint8_t *untiled,
                   uint8_t *tiled,
                   uint32_t firstSlice,
                   uint32_t numSlices)
{
   switch (info.tileMode) {
   case TileMode::Micro1DTiledThin1:
      retileTiledSurface2<IsUntiling, TileMode::Micro1DTiledThin1>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Micro1DTiledThick:
      retileTiledSurface2<IsUntiling, TileMode::Micro1DTiledThick>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2DTiledThin1:
      retileTiledSurface2<IsUntiling, TileMode::Macro2DTiledThin1>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2DTiledThin2:
      retileTiledSurface2<IsUntiling, TileMode::Macro2DTiledThin2>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2DTiledThin4:
      retileTiledSurface2<IsUntiling, TileMode::Macro2DTiledThin4>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2DTiledThick:
      retileTiledSurface2<IsUntiling, TileMode::Macro2DTiledThick>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2BTiledThin1:
      retileTiledSurface2<IsUntiling, TileMode::Macro2BTiledThin1>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2BTiledThin2:
      retileTiledSurface2<IsUntiling, TileMode::Macro2BTiledThin2>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2BTiledThin4:
      retileTiledSurface2<IsUntiling, TileMode::Macro2BTiledThin4>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro2BTiledThick:
      retileTiledSurface2<IsUntiling, TileMode::Macro2BTiledThick>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro3DTiledThin1:
      retileTiledSurface2<IsUntiling, TileMode::Macro3DTiledThin1>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro3DTiledThick:
      retileTiledSurface2<IsUntiling, TileMode::Macro3DTiledThick>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro3BTiledThin1:
      retileTiledSurface2<IsUntiling, TileMode::Macro3BTiledThin1>(info, untiled, tiled, firstSlice, numSlices);
      break;
   case TileMode::Macro3BTiledThick:
      retileTiledSurface2<IsUntiling, TileMode::Macro3BTiledThick>(info, untiled, tiled, firstSlice, numSlices);
      break;
   default:
      decaf_abort("Unexpected tiled tile mode");
   }
}

template<bool IsUntiling>
static void
retileLinearSurface(const RetileInfo& info,
                    uint8_t *untiled,
                    uint8_t *tiled,
                    uint32_t firstSlice,
                    uint32_t numSlices)
{
   // These functions assume that the tiled side is aligned to a thick slice
   // boundary, since this is linear, we just pull that pointer forward.
   uint32_t thinSliceAdjust = firstSlice % info.microTileThickness;
   uint32_t thinMicroTileBytes = info.thickMicroTileBytes / info.microTileThickness;
   tiled += thinSliceAdjust * info.numTilesPerSlice * thinMicroTileBytes;

   // Calculate the total number of tiles to copy.
   uint32_t totalTiles = numSlices * info.numTilesPerSlice;

   // Copy all the bytes from one place to the other.
   uint32_t totalBytes = totalTiles * thinMicroTileBytes;
   if (IsUntiling) {
      std::memcpy(untiled, tiled, totalBytes);
   } else {
      std::memcpy(tiled, untiled, totalBytes);
   }
}

template<bool IsUntiling>
static inline void
retile(const RetileInfo& info,
       uint8_t *untiled,
       uint8_t *tiled,
       uint32_t firstSlice,
       uint32_t numSlices)
{
   if (info.tileMode == TileMode::LinearGeneral ||
       info.tileMode == TileMode::LinearAligned) {
      return retileLinearSurface<IsUntiling>(info, untiled, tiled, firstSlice, numSlices);
   }

   retileTiledSurface<IsUntiling>(info, untiled, tiled, firstSlice, numSlices);
}

void
untile(const RetileInfo& info,
       uint8_t *untiled,
       uint8_t *tiled,
       uint32_t firstSlice,
       uint32_t numSlices)
{
   retile<true>(info, untiled, tiled, firstSlice, numSlices);
}

void
tile(const RetileInfo& info,
     uint8_t *untiled,
     uint8_t *tiled,
     uint32_t firstSlice,
     uint32_t numSlices)
{
   retile<false>(info, untiled, tiled, firstSlice, numSlices);
}

} // namespace gpu::tiling::cpu
