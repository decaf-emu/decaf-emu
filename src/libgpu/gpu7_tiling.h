#pragma once
#include <array>
#include <addrlib/addrinterface.h>

namespace gpu7::tiling
{

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
static constexpr auto SampleSplitSize = 2048;
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
getMacroTileWidth(AddrTileMode tileMode)
{
   return MacroTileWidth[static_cast<size_t>(tileMode)];
}

static constexpr int
getMacroTileHeight(AddrTileMode tileMode)
{
   return MacroTileHeight[static_cast<size_t>(tileMode)];
}

static constexpr int
getMicroTileThickness(AddrTileMode tileMode)
{
   return MicroTileThickness[static_cast<size_t>(tileMode)];
}

struct SurfaceDescription
{
   AddrTileMode tileMode;
   AddrFormat format;
   uint32_t bpp;
   uint32_t numSamples;
   uint32_t width;
   uint32_t height;
   uint32_t numSlices;
   ADDR_SURFACE_FLAGS flags;
   uint32_t numFrags;
   uint32_t numLevels;
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
};

typedef ADDR_COMPUTE_SURFACE_INFO_OUTPUT SurfaceInfo;
typedef AddrTileMode TileMode;

SurfaceInfo
computeSurfaceInfo(const SurfaceDescription &surface,
                   int mipLevel,
                   int slice);

int
computeSurfaceBankSwappedWidth(AddrTileMode tileMode,
                               uint32_t bpp,
                               uint32_t numSamples,
                               uint32_t pitch);

void
untileImage(const SurfaceDescription &surface,
            void *src,
            void *dst);

void
untileImageSlice(const SurfaceDescription &surface,
                 void *src,
                 void *dst,
                 int slice);

void
untileMipMap(const SurfaceDescription &desc,
             void *src,
             void *dst);

void
untileMip(const SurfaceDescription &surface,
          void *src,
          void *dst,
          int level);

void
untileMipSlice(const SurfaceDescription &desc,
               void *src,
               void *dst,
               int level,
               int slice);

size_t
computeUnpitchedImageSize(const SurfaceDescription &desc);

size_t
computeUnpitchedMipMapSize(const SurfaceDescription &desc);

void
unpitchImage(const SurfaceDescription &desc,
             void *pitched,
             void *unpitched);

void
unpitchMipMap(const SurfaceDescription &desc,
              void *pitched,
              void *unpitched);

} // namespace gpu7::tiling
