#include <common/decaf_assert.h>
#include "gpu_addrlibopt.h"
#include <algorithm>
#include <cstring>

namespace gpu
{

namespace addrlibopt
{

constexpr unsigned Log2(unsigned x)
{
   return x == 1 ? 0 : 1 + Log2(x >> 1);
}

constexpr uint32_t NumPipes = 2;
constexpr uint32_t NumBanks = 4;
constexpr uint32_t PipeInterleaveBytes = 256;
constexpr uint32_t RowSize = 2048;
constexpr uint32_t SwapSize = 256;
constexpr uint32_t SplitSize = 2048;
constexpr uint32_t UsesOptimalBankSwap = 0;

constexpr uint32_t MicroTileWidth = 8;
constexpr uint32_t MicroTileHeight = 8;
constexpr uint32_t ThickTileThickness = 4;
constexpr uint32_t XThickTileThickness = 8;
constexpr uint32_t HtileCacheBits = 16384;
constexpr uint32_t MicroTilePixels = MicroTileWidth * MicroTileHeight;

#define BITS_PER_BYTE 8
#define BITS_TO_BYTES(x) (((x) + (BITS_PER_BYTE-1)) / BITS_PER_BYTE)
#define BYTES_TO_BITS(x) ((x) * BITS_PER_BYTE)
#define _BIT(v,b) (((v) >> (b)) & 1)

constexpr uint32_t TileModeThickness[] = {
   /*ADDR_TM_LINEAR_GENERAL*/ 1,
   /*ADDR_TM_LINEAR_ALIGNED*/ 1,
   /*ADDR_TM_1D_TILED_THIN1*/ 1,
   /*ADDR_TM_1D_TILED_THICK*/ 4,
   /*ADDR_TM_2D_TILED_THIN1*/ 1,
   /*ADDR_TM_2D_TILED_THIN2*/ 1,
   /*ADDR_TM_2D_TILED_THIN4*/ 1,
   /*ADDR_TM_2D_TILED_THICK*/ 4,
   /*ADDR_TM_2B_TILED_THIN1*/ 1,
   /*ADDR_TM_2B_TILED_THIN2*/ 1,
   /*ADDR_TM_2B_TILED_THIN4*/ 1,
   /*ADDR_TM_2B_TILED_THICK*/ 4,
   /*ADDR_TM_3D_TILED_THIN1*/ 1,
   /*ADDR_TM_3D_TILED_THICK*/ 4,
   /*ADDR_TM_3B_TILED_THIN1*/ 1,
   /*ADDR_TM_3B_TILED_THICK*/ 4
};

template <AddrTileMode TileMode>
constexpr uint32_t
ComputeSurfaceThickness()
{
   return TileModeThickness[TileMode];
}

constexpr uint32_t TileMode2Rotation = NumPipes * ((NumBanks >> 1) - 1);
constexpr uint32_t TileMode3Rotation = (NumPipes >= 4) ? ((NumPipes >> 1) - 1) : 1;

constexpr uint32_t TileModeRotation[] = {
   /*ADDR_TM_LINEAR_GENERAL*/ 0,
   /*ADDR_TM_LINEAR_ALIGNED*/ 0,
   /*ADDR_TM_1D_TILED_THIN1*/ 0,
   /*ADDR_TM_1D_TILED_THICK*/ 0,
   /*ADDR_TM_2D_TILED_THIN1*/ TileMode2Rotation,
   /*ADDR_TM_2D_TILED_THIN2*/ TileMode2Rotation,
   /*ADDR_TM_2D_TILED_THIN4*/ TileMode2Rotation,
   /*ADDR_TM_2D_TILED_THICK*/ TileMode2Rotation,
   /*ADDR_TM_2B_TILED_THIN1*/ TileMode2Rotation,
   /*ADDR_TM_2B_TILED_THIN2*/ TileMode2Rotation,
   /*ADDR_TM_2B_TILED_THIN4*/ TileMode2Rotation,
   /*ADDR_TM_2B_TILED_THICK*/ TileMode2Rotation,
   /*ADDR_TM_3D_TILED_THIN1*/ TileMode3Rotation,
   /*ADDR_TM_3D_TILED_THICK*/ TileMode3Rotation,
   /*ADDR_TM_3B_TILED_THIN1*/ TileMode3Rotation,
   /*ADDR_TM_3B_TILED_THICK*/ TileMode3Rotation
};

template <AddrTileMode TileMode>
constexpr uint32_t
ComputeSurfaceRotationFromTileMode()
{
   return TileModeRotation[TileMode];
}

template <AddrTileMode TileMode>
constexpr bool
IsThickMacroTiled()
{
   static_assert(TileMode >= ADDR_TM_2D_TILED_THIN1 && TileMode <= ADDR_TM_3B_TILED_THICK,
      "Should only ever be called with a macro-tile tile mode");

   return ComputeSurfaceThickness<TileMode>() > 1;
}

constexpr bool TileModeBankSwapped[] = {
   /*ADDR_TM_LINEAR_GENERAL*/ false,
   /*ADDR_TM_LINEAR_ALIGNED*/ false,
   /*ADDR_TM_1D_TILED_THIN1*/ false,
   /*ADDR_TM_1D_TILED_THICK*/ false,
   /*ADDR_TM_2D_TILED_THIN1*/ false,
   /*ADDR_TM_2D_TILED_THIN2*/ false,
   /*ADDR_TM_2D_TILED_THIN4*/ false,
   /*ADDR_TM_2D_TILED_THICK*/ false,
   /*ADDR_TM_2B_TILED_THIN1*/ true,
   /*ADDR_TM_2B_TILED_THIN2*/ true,
   /*ADDR_TM_2B_TILED_THIN4*/ true,
   /*ADDR_TM_2B_TILED_THICK*/ true,
   /*ADDR_TM_3D_TILED_THIN1*/ false,
   /*ADDR_TM_3D_TILED_THICK*/ false,
   /*ADDR_TM_3B_TILED_THIN1*/ true,
   /*ADDR_TM_3B_TILED_THICK*/ true
};

template <AddrTileMode TileMode>
constexpr bool
IsBankSwappedTileMode()
{
   static_assert(TileMode >= ADDR_TM_2D_TILED_THIN1 && TileMode <= ADDR_TM_3B_TILED_THICK,
      "Should only ever be called with a macro-tile tile mode");

   return TileModeBankSwapped[TileMode];
}

constexpr uint32_t TileModeAspectRatio[] = {
   /*ADDR_TM_LINEAR_GENERAL*/ 0,
   /*ADDR_TM_LINEAR_ALIGNED*/ 0,
   /*ADDR_TM_1D_TILED_THIN1*/ 0,
   /*ADDR_TM_1D_TILED_THICK*/ 0,
   /*ADDR_TM_2D_TILED_THIN1*/ 1,
   /*ADDR_TM_2D_TILED_THIN2*/ 2,
   /*ADDR_TM_2D_TILED_THIN4*/ 4,
   /*ADDR_TM_2D_TILED_THICK*/ 1,
   /*ADDR_TM_2B_TILED_THIN1*/ 1,
   /*ADDR_TM_2B_TILED_THIN2*/ 2,
   /*ADDR_TM_2B_TILED_THIN4*/ 4,
   /*ADDR_TM_2B_TILED_THICK*/ 1,
   /*ADDR_TM_3D_TILED_THIN1*/ 1,
   /*ADDR_TM_3D_TILED_THICK*/ 1,
   /*ADDR_TM_3B_TILED_THIN1*/ 1,
   /*ADDR_TM_3B_TILED_THICK*/ 1
};

template <AddrTileMode TileMode>
constexpr uint32_t
ComputeMacroTileAspectRatio()
{
   static_assert(TileMode >= ADDR_TM_2D_TILED_THIN1 && TileMode <= ADDR_TM_3B_TILED_THICK,
      "Should only ever be called with a macro-tile tile mode");

   return TileModeAspectRatio[TileMode];
}

template <AddrTileMode TileMode>
constexpr uint64_t
ComputeMacroTilePitch()
{
   return (8 * NumBanks) / ComputeMacroTileAspectRatio<TileMode>();
}

template <AddrTileMode TileMode>
constexpr uint64_t
ComputeMacroTileHeight()
{
   return (8 * NumPipes) * ComputeMacroTileAspectRatio<TileMode>();
}

template <bool IsDepth>
constexpr AddrTileType
GetTileType()
{
   return IsDepth ? ADDR_NON_DISPLAYABLE : ADDR_DISPLAYABLE;
}

template<uint32_t Bpp, AddrTileMode TileMode, AddrTileType TileType>
static inline uint32_t
ComputePixelIndexWithinMicroTile(uint32_t x,
                                 uint32_t y,
                                 uint32_t z)
{
   uint32_t pixelBit0 = 0;
   uint32_t pixelBit1 = 0;
   uint32_t pixelBit2 = 0;
   uint32_t pixelBit3 = 0;
   uint32_t pixelBit4 = 0;
   uint32_t pixelBit5 = 0;
   uint32_t pixelBit6 = 0;
   uint32_t pixelBit7 = 0;
   uint32_t pixelBit8 = 0;
   uint32_t pixelNumber;

   uint32_t x0 = _BIT(x, 0);
   uint32_t x1 = _BIT(x, 1);
   uint32_t x2 = _BIT(x, 2);
   uint32_t y0 = _BIT(y, 0);
   uint32_t y1 = _BIT(y, 1);
   uint32_t y2 = _BIT(y, 2);
   uint32_t z0 = _BIT(z, 0);
   uint32_t z1 = _BIT(z, 1);
   uint32_t z2 = _BIT(z, 2);

   constexpr auto thickness = ComputeSurfaceThickness<TileMode>();

   if (TileType == ADDR_THICK_TILING) {
      pixelBit0 = x0;
      pixelBit1 = y0;
      pixelBit2 = z0;
      pixelBit3 = x1;
      pixelBit4 = y1;
      pixelBit5 = z1;
      pixelBit6 = x2;
      pixelBit7 = y2;
   } else {
      if (TileType == ADDR_NON_DISPLAYABLE) {
         pixelBit0 = x0;
         pixelBit1 = y0;
         pixelBit2 = x1;
         pixelBit3 = y1;
         pixelBit4 = x2;
         pixelBit5 = y2;
      } else {
         switch (Bpp) {
         case 8:
            pixelBit0 = x0;
            pixelBit1 = x1;
            pixelBit2 = x2;
            pixelBit3 = y1;
            pixelBit4 = y0;
            pixelBit5 = y2;
            break;
         case 16:
            pixelBit0 = x0;
            pixelBit1 = x1;
            pixelBit2 = x2;
            pixelBit3 = y0;
            pixelBit4 = y1;
            pixelBit5 = y2;
            break;
         case 64:
            pixelBit0 = x0;
            pixelBit1 = y0;
            pixelBit2 = x1;
            pixelBit3 = x2;
            pixelBit4 = y1;
            pixelBit5 = y2;
            break;
         case 128:
            pixelBit0 = y0;
            pixelBit1 = x0;
            pixelBit2 = x1;
            pixelBit3 = x2;
            pixelBit4 = y1;
            pixelBit5 = y2;
            break;
         case 32:
         case 96:
         default:
            pixelBit0 = x0;
            pixelBit1 = x1;
            pixelBit2 = y0;
            pixelBit3 = x2;
            pixelBit4 = y1;
            pixelBit5 = y2;
            break;
         }
      }

      if (thickness > 1) {
         pixelBit6 = z0;
         pixelBit7 = z1;
      }
   }

   if (thickness == 8) {
      pixelBit8 = z2;
   }

   pixelNumber = ((pixelBit0) |
      (pixelBit1 << 1) |
      (pixelBit2 << 2) |
      (pixelBit3 << 3) |
      (pixelBit4 << 4) |
      (pixelBit5 << 5) |
      (pixelBit6 << 6) |
      (pixelBit7 << 7) |
      (pixelBit8 << 8));

   return pixelNumber;
}

static inline uint32_t
ComputePipeFromCoordWoRotation(uint32_t x,
                               uint32_t y)
{
   uint32_t pipe;
   uint32_t pipeBit0 = 0;
   uint32_t pipeBit1 = 0;
   uint32_t pipeBit2 = 0;
   uint32_t pipeBit3 = 0;

   uint32_t x3 = _BIT(x, 3);
   uint32_t x4 = _BIT(x, 4);
   uint32_t x5 = _BIT(x, 5);
   uint32_t y3 = _BIT(y, 3);
   uint32_t y4 = _BIT(y, 4);
   uint32_t y5 = _BIT(y, 5);

   switch (NumPipes) {
   case 1:
      pipeBit0 = 0;
      break;
   case 2:
      pipeBit0 = (y3 ^ x3);
      break;
   case 4:
      pipeBit0 = (y3 ^ x4);
      pipeBit1 = (y4 ^ x3);
      break;
   case 8:
      pipeBit0 = (y3 ^ x5);
      pipeBit1 = (y4 ^ x5 ^ x4);
      pipeBit2 = (y5 ^ x3);
      break;
   default:
      pipe = 0;
      break;
   }

   pipe = pipeBit0 | (pipeBit1 << 1) | (pipeBit2 << 2);
   return pipe;
}

static inline uint32_t
ComputeBankFromCoordWoRotation(uint32_t x,
                               uint32_t y)
{
   uint32_t tx = x / NumBanks;
   uint32_t ty = y / NumPipes;

   uint32_t bank;
   uint32_t bankBit0 = 0;
   uint32_t bankBit1 = 0;
   uint32_t bankBit2 = 0;

   uint32_t x3 = _BIT(x, 3);
   uint32_t x4 = _BIT(x, 4);
   uint32_t x5 = _BIT(x, 5);

   uint32_t tx3 = _BIT(tx, 3);

   uint32_t ty3 = _BIT(ty, 3);
   uint32_t ty4 = _BIT(ty, 4);
   uint32_t ty5 = _BIT(ty, 5);

   switch (NumBanks)
   {
   case 4:
      bankBit0 = (ty4 ^ x3);

      if (UsesOptimalBankSwap == 1 && NumPipes == 8) {
         bankBit0 ^= x5;
      }

      bankBit1 = (ty3 ^ x4);
      break;
   case 8:
      bankBit0 = (ty5 ^ x3);

      if (UsesOptimalBankSwap == 1 && NumPipes == 8) {
         bankBit0 ^= tx3;
      }

      bankBit1 = (ty5 ^ ty4 ^ x4);
      bankBit2 = (ty3 ^ x5);
      break;
   }

   bank = bankBit0 | (bankBit1 << 1) | (bankBit2 << 2);
   return bank;
}

template <uint32_t Bpp, AddrTileMode TileMode, uint32_t NumSamples>
static inline uint32_t
ComputeSurfaceBankSwappedWidth(uint32_t pitch)
{
   static_assert(IsBankSwappedTileMode<TileMode>(), "Should only be called with bank swapped tile modes");

   constexpr auto groupSize = PipeInterleaveBytes;
   constexpr auto bytesPerSample = 8 * Bpp;
   constexpr auto samplesPerTile = SplitSize / bytesPerSample;

   constexpr uint32_t numSamples = IsThickMacroTiled<TileMode>() ? 4 : NumSamples;
   constexpr uint32_t slicesPerTile = (SplitSize / bytesPerSample) ? std::max<uint32_t>(1u, NumSamples / samplesPerTile) : 1;
   constexpr auto bytesPerTileSlice = numSamples * bytesPerSample / slicesPerTile;

   constexpr auto factor = ComputeMacroTileAspectRatio<TileMode>();
   constexpr auto swapTiles = std::max<uint32_t>(1u, (SwapSize >> 1) / Bpp);
   constexpr auto swapWidth = swapTiles * 8 * NumBanks;
   constexpr auto heightBytes = NumSamples * factor * NumPipes * Bpp / slicesPerTile;
   constexpr auto swapMax = NumPipes * NumBanks * RowSize / heightBytes;
   constexpr auto swapMin = groupSize * 8 * NumBanks / bytesPerTileSlice;

   constexpr auto bankSwapWidthBase = std::min(swapMax, std::max(swapMin, swapWidth));

   uint32_t bankSwapWidth = bankSwapWidthBase;

   while (bankSwapWidth >= 2 * pitch) {
      bankSwapWidth >>= 1;
   }

   return bankSwapWidth;
}

template<uint32_t Bpp>
static uint64_t
ComputeSurfaceAddrFromCoordLinear(uint32_t x,
                                  uint32_t y,
                                  uint32_t slice,
                                  uint32_t sample,
                                  uint32_t pitch,
                                  uint32_t height,
                                  uint32_t numSlices)
{
   auto sliceSize = static_cast<uint64_t>(pitch) * height;

   auto sliceOffset = sliceSize * (slice + sample * numSlices);
   auto rowOffset = static_cast<uint64_t>(y) * pitch;
   auto pixOffset = static_cast<uint64_t>(x);

   auto addr = (sliceOffset + rowOffset + pixOffset) * Bpp;

   addr /= 8;

   return addr;
}

template<bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
static uint64_t
ComputeSurfaceAddrFromCoordMicroTiled(uint32_t x,
                                      uint32_t y,
                                      uint32_t slice,
                                      uint32_t pitch,
                                      uint32_t height,
                                      uint32_t tileBase,
                                      uint32_t compBits)
{
   constexpr uint64_t microTileThickness = ComputeSurfaceThickness<TileMode>();

   constexpr uint64_t microTileBytes = BITS_TO_BYTES(MicroTilePixels * microTileThickness * Bpp);
   uint64_t microTilesPerRow = pitch / MicroTileWidth;
   uint64_t microTileIndexX = x / MicroTileWidth;
   uint64_t microTileIndexY = y / MicroTileHeight;
   uint64_t microTileIndexZ = slice / microTileThickness;

   uint64_t microTileOffset = microTileBytes * (microTileIndexX + microTileIndexY * microTilesPerRow);

   uint64_t sliceBytes = BITS_TO_BYTES(pitch * height * microTileThickness * Bpp);
   uint64_t sliceOffset = microTileIndexZ * sliceBytes;

   uint64_t pixelIndex = ComputePixelIndexWithinMicroTile<Bpp, TileMode, GetTileType<IsDepth>()>(x, y, slice);
   uint64_t pixelOffset;

   if (compBits && compBits != Bpp && IsDepth) {
      pixelOffset = tileBase + compBits * pixelIndex;
   } else {
      pixelOffset = Bpp * pixelIndex;
   }

   pixelOffset /= 8;

   return pixelOffset + microTileOffset + sliceOffset;
}

template<bool IsBankSwapped>
struct DispatchGetSwappedBank {
};

template<>
struct DispatchGetSwappedBank<false> {
   template<uint32_t Bpp, AddrTileMode TileMode, uint32_t NumSamples>
   static inline uint64_t
   call(uint64_t bank, uint32_t pitch, uint64_t macroTileIndexX)
   {
      return bank;
   }
};

template<>
struct DispatchGetSwappedBank<true> {
   template<uint32_t Bpp, AddrTileMode TileMode, uint32_t NumSamples>
   static inline uint64_t
   call(uint64_t bank, uint32_t pitch, uint64_t macroTileIndexX)
   {
      constexpr uint64_t macroTilePitch = ComputeMacroTilePitch<TileMode>();
      constexpr uint32_t bankSwapOrder[] = { 0, 1, 3, 2, 6, 7, 5, 4, 0, 0 };
      uint64_t bankSwapWidth = ComputeSurfaceBankSwappedWidth<Bpp, TileMode, NumSamples>(pitch);
      uint64_t swapIndex = macroTilePitch * macroTileIndexX / bankSwapWidth;
      bank ^= bankSwapOrder[swapIndex & (NumBanks - 1)];
      return bank;
   }
};

template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
static uint64_t
ComputeSurfaceAddrFromCoordMacroTiled(uint32_t x,
                                      uint32_t y,
                                      uint32_t slice,
                                      uint32_t sample,
                                      uint32_t pitch,
                                      uint32_t height,
                                      uint32_t tileBase,
                                      uint32_t compBits,
                                      uint32_t pipeSwizzle,
                                      uint32_t bankSwizzle)
{
   constexpr uint64_t numGroupBits = Log2(PipeInterleaveBytes);
   constexpr uint64_t numPipeBits = Log2(NumPipes);
   constexpr uint64_t numBankBits = Log2(NumBanks);

   constexpr uint64_t microTileThickness = ComputeSurfaceThickness<TileMode>();
   constexpr uint64_t microTileBits = MicroTilePixels * microTileThickness * Bpp * NumSamples;
   constexpr uint64_t microTileBytes = microTileBits / 8;

   constexpr uint64_t bytesPerSample = microTileBytes / NumSamples;
   constexpr bool IsSamplesSplit = NumSamples > 1 && microTileBytes > static_cast<uint64_t>(SplitSize);
   constexpr uint64_t _samplesPerSlice = IsSamplesSplit ? SplitSize / bytesPerSample : NumSamples;
   constexpr uint64_t samplesPerSlice = _samplesPerSlice > 0 ? _samplesPerSlice : 1;
   constexpr uint64_t numSampleSplits = IsSamplesSplit ? NumSamples / samplesPerSlice : 1;
   constexpr uint32_t numSurfSamples = IsSamplesSplit ? static_cast<uint32_t>(samplesPerSlice) : NumSamples;

   constexpr uint64_t rotation = ComputeSurfaceRotationFromTileMode<TileMode>();
   constexpr uint64_t macroTilePitch = ComputeMacroTilePitch<TileMode>();
   constexpr uint64_t macroTileHeight = ComputeMacroTileHeight<TileMode>();
   constexpr uint64_t group_mask = (1 << numGroupBits) - 1;

   uint64_t pixelIndex = ComputePixelIndexWithinMicroTile<Bpp, TileMode, GetTileType<IsDepth>()>(x, y, slice);

   uint64_t sampleOffset;
   uint64_t pixelOffset;

   if (IsDepth) {
      if (compBits && compBits != Bpp) {
         sampleOffset = tileBase + compBits * sample;
         pixelOffset = NumSamples * compBits * pixelIndex;
      } else {
         sampleOffset = Bpp * sample;
         pixelOffset = NumSamples * Bpp * pixelIndex;
      }
   } else {
      sampleOffset = sample * (microTileBits / NumSamples);
      pixelOffset = Bpp * pixelIndex;
   }

   uint64_t elemOffset = pixelOffset + sampleOffset;

   uint64_t sampleSlice;

   if (IsSamplesSplit) {
      constexpr uint64_t tileSliceBits = microTileBits / numSampleSplits;
      sampleSlice = elemOffset / tileSliceBits;
      elemOffset %= tileSliceBits;
   } else {
      sampleSlice = 0;
   }

   elemOffset /= 8;

   uint64_t pipe = ComputePipeFromCoordWoRotation(x, y);
   uint64_t bank = ComputeBankFromCoordWoRotation(x, y);

   uint64_t bankPipe = pipe + NumPipes * bank;
   uint64_t swizzle = pipeSwizzle + NumPipes * bankSwizzle;
   uint64_t sliceIn = slice;

   if (IsThickMacroTiled<TileMode>()) {
      sliceIn /= ThickTileThickness;
   }

   bankPipe ^= NumPipes * sampleSlice * ((NumBanks >> 1) + 1) ^ (swizzle + sliceIn * rotation);
   bankPipe %= NumPipes * NumBanks;
   pipe = bankPipe % NumPipes;
   bank = bankPipe / NumPipes;

   uint64_t sliceBytes = BITS_TO_BYTES(pitch * height * microTileThickness * Bpp * numSurfSamples);
   uint64_t sliceOffset = sliceBytes * ((sampleSlice + numSampleSplits * slice) / microTileThickness);

   uint64_t macroTilesPerRow = pitch / macroTilePitch;
   uint64_t macroTileBytes = BITS_TO_BYTES(numSurfSamples * microTileThickness * Bpp * macroTileHeight * macroTilePitch);
   uint64_t macroTileIndexX = x / macroTilePitch;
   uint64_t macroTileIndexY = y / macroTileHeight;
   uint64_t macroTileOffset = macroTileBytes * (macroTileIndexX + macroTilesPerRow * macroTileIndexY);

   // Do bank swapping if needed
   using BankSwapStruct = DispatchGetSwappedBank<IsBankSwappedTileMode<TileMode>()>;
   bank = BankSwapStruct::template call<Bpp, TileMode, numSurfSamples>(bank, pitch, macroTileIndexX);

   // Calculate final offset
   uint64_t total_offset = elemOffset + ((macroTileOffset + sliceOffset) >> (numBankBits + numPipeBits));
   uint64_t offset_high = (total_offset & ~group_mask) << (numBankBits + numPipeBits);
   uint64_t offset_low = total_offset & group_mask;
   uint64_t bank_bits = bank << (numPipeBits + numGroupBits);
   uint64_t pipe_bits = pipe << numGroupBits;
   uint64_t offset = bank_bits | pipe_bits | offset_low | offset_high;

   return offset;
}

enum class TilingMode : uint32_t
{
   Linear,
   Micro,
   Macro
};

constexpr TilingMode TileModeTiling[] = {
   /*ADDR_TM_LINEAR_GENERAL*/ TilingMode::Linear,
   /*ADDR_TM_LINEAR_ALIGNED*/ TilingMode::Linear,
   /*ADDR_TM_1D_TILED_THIN1*/ TilingMode::Micro,
   /*ADDR_TM_1D_TILED_THICK*/ TilingMode::Micro,
   /*ADDR_TM_2D_TILED_THIN1*/ TilingMode::Macro,
   /*ADDR_TM_2D_TILED_THIN2*/ TilingMode::Macro,
   /*ADDR_TM_2D_TILED_THIN4*/ TilingMode::Macro,
   /*ADDR_TM_2D_TILED_THICK*/ TilingMode::Macro,
   /*ADDR_TM_2B_TILED_THIN1*/ TilingMode::Macro,
   /*ADDR_TM_2B_TILED_THIN2*/ TilingMode::Macro,
   /*ADDR_TM_2B_TILED_THIN4*/ TilingMode::Macro,
   /*ADDR_TM_2B_TILED_THICK*/ TilingMode::Macro,
   /*ADDR_TM_3D_TILED_THIN1*/ TilingMode::Macro,
   /*ADDR_TM_3D_TILED_THICK*/ TilingMode::Macro,
   /*ADDR_TM_3B_TILED_THIN1*/ TilingMode::Macro,
   /*ADDR_TM_3B_TILED_THICK*/ TilingMode::Macro
};

template <TilingMode TilingModeTiling>
struct DispatchAddrComputeSurfaceAddrFromCoord {
};

template <>
struct DispatchAddrComputeSurfaceAddrFromCoord<TilingMode::Linear> {
   template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
   static inline void
   call(const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *pIn,
        ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT *pOut)
   {
      pOut->addr = ComputeSurfaceAddrFromCoordLinear<Bpp>(pIn->x,
         pIn->y,
         pIn->slice,
         pIn->sample,
         pIn->pitch,
         pIn->height,
         pIn->numSlices);
   }
};

template <>
struct DispatchAddrComputeSurfaceAddrFromCoord<TilingMode::Micro> {
   template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
   static inline void
   call(const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *pIn,
        ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT *pOut)
   {
      pOut->addr = ComputeSurfaceAddrFromCoordMicroTiled<IsDepth, Bpp, TileMode>(pIn->x,
         pIn->y,
         pIn->slice,
         pIn->pitch,
         pIn->height,
         pIn->tileBase,
         pIn->compBits);
   }
};

template <>
struct DispatchAddrComputeSurfaceAddrFromCoord<TilingMode::Macro> {
   template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
   static inline void
   call(const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *pIn,
        ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT *pOut)
   {
      pOut->addr = ComputeSurfaceAddrFromCoordMacroTiled<NumSamples, IsDepth, Bpp, TileMode>(pIn->x,
         pIn->y,
         pIn->slice,
         pIn->sample,
         pIn->pitch,
         pIn->height,
         pIn->tileBase,
         pIn->compBits,
         pIn->pipeSwizzle,
         pIn->bankSwizzle);
   }
};

template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp, AddrTileMode TileMode>
static void
AddrComputeSurfaceAddrFromCoord(const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *pIn,
                                ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT *pOut)
{
   using TileModeStruct = DispatchAddrComputeSurfaceAddrFromCoord<TileModeTiling[TileMode]>;
   TileModeStruct::template call<NumSamples, IsDepth, Bpp, TileMode>(pIn, pOut);
}

typedef void(*AddrFromCoordFunc)(const ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT *pIn,
                                 ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT *pOut);

template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp>
static bool
copySurfacePixels6(uint8_t *dstBasePtr,
                   uint32_t dstWidth,
                   uint32_t dstHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                   uint8_t *srcBasePtr,
                   uint32_t srcWidth,
                   uint32_t srcHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                   AddrFromCoordFunc dstCoordFunc,
                   AddrFromCoordFunc srcCoordFunc)
{
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT srcAddrOutput;
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT dstAddrOutput;

   std::memset(&srcAddrOutput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT));
   std::memset(&dstAddrOutput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT));

   srcAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);
   dstAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);

   for (auto y = 0u; y < dstHeight; ++y) {
      for (auto x = 0u; x < dstWidth; ++x) {
         srcAddrInput.x = srcWidth * x / dstWidth;
         srcAddrInput.y = srcHeight * y / dstHeight;
         srcCoordFunc(&srcAddrInput, &srcAddrOutput);

         dstAddrInput.x = x;
         dstAddrInput.y = y;
         dstCoordFunc(&dstAddrInput, &dstAddrOutput);

         auto src = &srcBasePtr[srcAddrOutput.addr];
         auto dst = &dstBasePtr[dstAddrOutput.addr];
         std::memcpy(dst, src, Bpp / 8);
      }
   }

   return true;
}

// Selects source tile mode template
template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp>
static bool
copySurfacePixels5(uint8_t *dstBasePtr,
                   uint32_t dstWidth,
                   uint32_t dstHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                   uint8_t *srcBasePtr,
                   uint32_t srcWidth,
                   uint32_t srcHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                   AddrFromCoordFunc dstCoordFunc)
{
   AddrFromCoordFunc srcCoordFunc = nullptr;

   switch (srcAddrInput.tileMode) {
      // We drop the distinction between linear tile modes here since it doesn't affect
      //  the end result but removes one permutation of templates...
   case ADDR_TM_LINEAR_GENERAL:
   case ADDR_TM_LINEAR_ALIGNED:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_LINEAR_GENERAL>;
      break;
   case ADDR_TM_1D_TILED_THIN1:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_1D_TILED_THIN1>;
      break;
   case ADDR_TM_1D_TILED_THICK:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_1D_TILED_THICK>;
      break;
   case ADDR_TM_2D_TILED_THIN1:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN1>;
      break;
   case ADDR_TM_2D_TILED_THIN2:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN2>;
      break;
   case ADDR_TM_2D_TILED_THIN4:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN4>;
      break;
   case ADDR_TM_2D_TILED_THICK:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THICK>;
      break;
   case ADDR_TM_2B_TILED_THIN1:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN1>;
      break;
   case ADDR_TM_2B_TILED_THIN2:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN2>;
      break;
   case ADDR_TM_2B_TILED_THIN4:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN4>;
      break;
   case ADDR_TM_2B_TILED_THICK:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THICK>;
      break;
   case ADDR_TM_3D_TILED_THIN1:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3D_TILED_THIN1>;
      break;
   case ADDR_TM_3D_TILED_THICK:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3D_TILED_THICK>;
      break;
   case ADDR_TM_3B_TILED_THIN1:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3B_TILED_THIN1>;
      break;
   case ADDR_TM_3B_TILED_THICK:
      srcCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3B_TILED_THICK>;
      break;
   default:
      decaf_abort("Unexpected destination tiling type");
   }

   return copySurfacePixels6<NumSamples, IsDepth, Bpp>(
      dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, dstCoordFunc, srcCoordFunc);
}

// Selects destination tile mode template
template<uint32_t NumSamples, bool IsDepth, uint32_t Bpp>
static bool
copySurfacePixels4(uint8_t *dstBasePtr,
                   uint32_t dstWidth,
                   uint32_t dstHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                   uint8_t *srcBasePtr,
                   uint32_t srcWidth,
                   uint32_t srcHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput)
{
   AddrFromCoordFunc dstCoordFunc = nullptr;

   switch (dstAddrInput.tileMode) {
      // We drop the distinction between linear tile modes here since it doesn't affect
      //  the end result but removes one permutation of templates...
   case ADDR_TM_LINEAR_GENERAL:
   case ADDR_TM_LINEAR_ALIGNED:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_LINEAR_GENERAL>;
      break;
   case ADDR_TM_1D_TILED_THIN1:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_1D_TILED_THIN1>;
      break;
   case ADDR_TM_1D_TILED_THICK:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_1D_TILED_THICK>;
      break;
   case ADDR_TM_2D_TILED_THIN1:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN1>;
      break;
   case ADDR_TM_2D_TILED_THIN2:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN2>;
      break;
   case ADDR_TM_2D_TILED_THIN4:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THIN4>;
      break;
   case ADDR_TM_2D_TILED_THICK:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2D_TILED_THICK>;
      break;
   case ADDR_TM_2B_TILED_THIN1:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN1>;
      break;
   case ADDR_TM_2B_TILED_THIN2:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN2>;
      break;
   case ADDR_TM_2B_TILED_THIN4:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THIN4>;
      break;
   case ADDR_TM_2B_TILED_THICK:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_2B_TILED_THICK>;
      break;
   case ADDR_TM_3D_TILED_THIN1:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3D_TILED_THIN1>;
      break;
   case ADDR_TM_3D_TILED_THICK:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3D_TILED_THICK>;
      break;
   case ADDR_TM_3B_TILED_THIN1:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3B_TILED_THIN1>;
      break;
   case ADDR_TM_3B_TILED_THICK:
      dstCoordFunc = &AddrComputeSurfaceAddrFromCoord<NumSamples, IsDepth, Bpp, ADDR_TM_3B_TILED_THICK>;
      break;
   default:
      decaf_abort("Unexpected destination tiling type");
   }

   return copySurfacePixels5<NumSamples, IsDepth, Bpp>(
      dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, dstCoordFunc);
}

// Optimized for copying between linear buffers
template<uint32_t Bpp>
static bool
copySurfacePixelsLinear(uint8_t *dstBasePtr,
                        uint32_t dstWidth,
                        uint32_t dstHeight,
                        ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                        uint8_t *srcBasePtr,
                        uint32_t srcWidth,
                        uint32_t srcHeight,
                        ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput)
{
   auto srcBaseAddr = ComputeSurfaceAddrFromCoordLinear<Bpp>(
      0,
      0,
      srcAddrInput.slice,
      srcAddrInput.sample,
      srcAddrInput.pitch,
      srcAddrInput.height,
      srcAddrInput.numSlices);
   auto dstBaseAddr = ComputeSurfaceAddrFromCoordLinear<Bpp>(
      0,
      0,
      dstAddrInput.slice,
      dstAddrInput.sample,
      dstAddrInput.pitch,
      dstAddrInput.height,
      dstAddrInput.numSlices);

   constexpr auto bytesPerPixel = Bpp / 8;
   auto src = &srcBasePtr[srcBaseAddr];
   auto dst = &dstBasePtr[dstBaseAddr];
   auto srcPitch = srcAddrInput.pitch * bytesPerPixel;
   auto dstPitch = dstAddrInput.pitch * bytesPerPixel;
   auto srcXInc = (static_cast<uint64_t>(srcWidth) << 32) / dstWidth;
   auto srcYInc = (static_cast<uint64_t>(srcHeight) << 32) / dstHeight;

   uint64_t srcYFrac = 0;
   for (auto y = 0u; y < dstHeight; ++y, dst += dstPitch, srcYFrac += srcYInc) {
      auto srcY = static_cast<uint32_t>(srcYFrac >> 32);
      auto srcRow = &src[srcY * srcPitch];
      uint64_t srcXFrac = 0;
      for (auto x = 0u; x < dstWidth; ++x, srcXFrac += srcXInc) {
         auto srcX = static_cast<uint32_t>(srcXFrac >> 32);
         std::memcpy(&dst[x * bytesPerPixel],
                     &srcRow[srcX * bytesPerPixel],
                     bytesPerPixel);
      }
   }

   return true;
}

// Selects Bpp template
template<uint32_t NumSamples, bool IsDepth>
static bool
copySurfacePixels3(uint8_t *dstBasePtr,
                   uint32_t dstWidth,
                   uint32_t dstHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                   uint8_t *srcBasePtr,
                   uint32_t srcWidth,
                   uint32_t srcHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                   uint32_t bpp)
{
   switch (bpp) {
   case 8:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<8>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 8>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   case 16:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<16>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 16>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   case 32:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<32>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 32>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   case 64:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<64>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 64>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   case 96:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<96>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 96>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   case 128:
      if (TileModeTiling[dstAddrInput.tileMode] == TilingMode::Linear
       && TileModeTiling[srcAddrInput.tileMode] == TilingMode::Linear) {
         return copySurfacePixelsLinear<128>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      } else {
         return copySurfacePixels4<NumSamples, IsDepth, 128>(
            dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput);
      }
   default:
      decaf_abort("Unexpected bits-per-pixel value");
   }
}

// Selects IsDepth template
template<uint32_t NumSamples>
static bool
copySurfacePixels2(uint8_t *dstBasePtr,
                   uint32_t dstWidth,
                   uint32_t dstHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                   uint8_t *srcBasePtr,
                   uint32_t srcWidth,
                   uint32_t srcHeight,
                   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                   uint32_t bpp,
                   bool isDepth)
{
   if (isDepth) {
      return copySurfacePixels3<NumSamples, true>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp);
   } else {
      return copySurfacePixels3<NumSamples, false>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp);
   }
}

// Selects NumSamples template
bool
copySurfacePixels(uint8_t *dstBasePtr,
                  uint32_t dstWidth,
                  uint32_t dstHeight,
                  ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
                  uint8_t *srcBasePtr,
                  uint32_t srcWidth,
                  uint32_t srcHeight,
                  ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                  uint32_t bpp,
                  bool isDepth,
                  uint32_t numSamples)
{
   switch (numSamples) {
   case 1:
      return copySurfacePixels2<1>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp, isDepth);
   case 2:
      return copySurfacePixels2<2>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp, isDepth);
   case 4:
      return copySurfacePixels2<4>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp, isDepth);
   case 8:
      return copySurfacePixels2<8>(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput, srcBasePtr, srcWidth, srcHeight, srcAddrInput, bpp, isDepth);
   default:
      decaf_abort("Unexpected number of samples value");
   }
}

} // namespace addrlibopt

} // namespace gpu
