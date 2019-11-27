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

#include <common/enum_start.inl>

ENUM_BEG(SurfaceDim, uint32_t)
   ENUM_VALUE(Texture1D,               0)
   ENUM_VALUE(Texture2D,               1)
   ENUM_VALUE(Texture3D,               2)
   ENUM_VALUE(TextureCube,             3)
   ENUM_VALUE(Texture1DArray,          4)
   ENUM_VALUE(Texture2DArray,          5)
   ENUM_VALUE(Texture2DMSAA,           6)
   ENUM_VALUE(Texture2DMSAAArray,      7)
ENUM_END(SurfaceDim)

FLAGS_BEG(SurfaceUse, uint32_t)
   FLAGS_VALUE(None,             0)
   FLAGS_VALUE(Texture,          1 << 0)
   FLAGS_VALUE(ColorBuffer,      1 << 1)
   FLAGS_VALUE(DepthBuffer,      1 << 2)
   FLAGS_VALUE(ScanBuffer,       1 << 3)
FLAGS_END(SurfaceUse)

#include <common/enum_end.inl>

struct SurfaceDescription
{
   AddrTileMode tileMode = ADDR_TM_LINEAR_GENERAL;
   AddrFormat format = ADDR_FMT_INVALID;
   uint32_t bpp = 0u;
   uint32_t numSamples = 0u;
   uint32_t width = 0u;
   uint32_t height = 0u;
   uint32_t numSlices = 0u;
   uint32_t numFrags = 0u;
   uint32_t numLevels = 0u;
   uint32_t bankSwizzle = 0u;
   uint32_t pipeSwizzle = 0u;
   SurfaceUse use = SurfaceUse::None;
   SurfaceDim dim = SurfaceDim::Texture1D;
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
