#pragma once
#include <array>

namespace gpu7::tiling
{

#include <common/enum_start.inl>

ENUM_BEG(DataFormat, uint32_t)
   ENUM_VALUE(Invalid,                    0)
   ENUM_VALUE(FMT_8,                      1)
   ENUM_VALUE(FMT_4_4,                    2)
   ENUM_VALUE(FMT_3_3_2,                  3)
   ENUM_VALUE(FMT_16,                     5)
   ENUM_VALUE(FMT_16_FLOAT,               6)
   ENUM_VALUE(FMT_8_8,                    7)
   ENUM_VALUE(FMT_5_6_5,                  8)
   ENUM_VALUE(FMT_6_5_5,                  9)
   ENUM_VALUE(FMT_1_5_5_5,                10)
   ENUM_VALUE(FMT_4_4_4_4,                11)
   ENUM_VALUE(FMT_5_5_5_1,                12)
   ENUM_VALUE(FMT_32,                     13)
   ENUM_VALUE(FMT_32_FLOAT,               14)
   ENUM_VALUE(FMT_16_16,                  15)
   ENUM_VALUE(FMT_16_16_FLOAT,            16)
   ENUM_VALUE(FMT_8_24,                   17)
   ENUM_VALUE(FMT_8_24_FLOAT,             18)
   ENUM_VALUE(FMT_24_8,                   19)
   ENUM_VALUE(FMT_24_8_FLOAT,             20)
   ENUM_VALUE(FMT_10_11_11,               21)
   ENUM_VALUE(FMT_10_11_11_FLOAT,         22)
   ENUM_VALUE(FMT_11_11_10,               23)
   ENUM_VALUE(FMT_11_11_10_FLOAT,         24)
   ENUM_VALUE(FMT_2_10_10_10,             25)
   ENUM_VALUE(FMT_8_8_8_8,                26)
   ENUM_VALUE(FMT_10_10_10_2,             27)
   ENUM_VALUE(FMT_X24_8_32_FLOAT,         28)
   ENUM_VALUE(FMT_32_32,                  29)
   ENUM_VALUE(FMT_32_32_FLOAT,            30)
   ENUM_VALUE(FMT_16_16_16_16,            31)
   ENUM_VALUE(FMT_16_16_16_16_FLOAT,      32)
   ENUM_VALUE(FMT_32_32_32_32,            34)
   ENUM_VALUE(FMT_32_32_32_32_FLOAT,      35)
   ENUM_VALUE(FMT_1,                      37)
   ENUM_VALUE(FMT_GB_GR,                  39)
   ENUM_VALUE(FMT_BG_RG,                  40)
   ENUM_VALUE(FMT_32_AS_8,                41)
   ENUM_VALUE(FMT_32_AS_8_8,              42)
   ENUM_VALUE(FMT_5_9_9_9_SHAREDEXP,      43)
   ENUM_VALUE(FMT_8_8_8,                  44)
   ENUM_VALUE(FMT_16_16_16,               45)
   ENUM_VALUE(FMT_16_16_16_FLOAT,         46)
   ENUM_VALUE(FMT_32_32_32,               47)
   ENUM_VALUE(FMT_32_32_32_FLOAT,         48)
   ENUM_VALUE(FMT_BC1,                    49)
   ENUM_VALUE(FMT_BC2,                    50)
   ENUM_VALUE(FMT_BC3,                    51)
   ENUM_VALUE(FMT_BC4,                    52)
   ENUM_VALUE(FMT_BC5,                    53)
   ENUM_VALUE(FMT_APC0,                   54)
   ENUM_VALUE(FMT_APC1,                   55)
   ENUM_VALUE(FMT_APC2,                   56)
   ENUM_VALUE(FMT_APC3,                   57)
   ENUM_VALUE(FMT_APC4,                   58)
   ENUM_VALUE(FMT_APC5,                   59)
   ENUM_VALUE(FMT_APC6,                   60)
   ENUM_VALUE(FMT_APC7,                   61)
   ENUM_VALUE(FMT_CTX1,                   62)
ENUM_END(DataFormat)

// TODO(brett19): Use this...
ENUM_BEG(TileMode, uint32_t)
   ENUM_VALUE(LinearGeneral,        0x00)
   ENUM_VALUE(LinearAligned,        0x01)
   ENUM_VALUE(Micro1DTiledThin1,    0x02)
   ENUM_VALUE(Micro1DTiledThick,    0x03)
   ENUM_VALUE(Macro2DTiledThin1,    0x04)
   ENUM_VALUE(Macro2DTiledThin2,    0x05)
   ENUM_VALUE(Macro2DTiledThin4,    0x06)
   ENUM_VALUE(Macro2DTiledThick,    0x07)
   ENUM_VALUE(Macro2BTiledThin1,    0x08)
   ENUM_VALUE(Macro2BTiledThin2,    0x09)
   ENUM_VALUE(Macro2BTiledThin4,    0x0A)
   ENUM_VALUE(Macro2BTiledThick,    0x0B)
   ENUM_VALUE(Macro3DTiledThin1,    0x0C)
   ENUM_VALUE(Macro3DTiledThick,    0x0D)
   ENUM_VALUE(Macro3BTiledThin1,    0x0E)
   ENUM_VALUE(Macro3BTiledThick,    0x0F)
   ENUM_VALUE(LinearSpecial,        0x10)
ENUM_END(TileMode)

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

static constexpr bool TileModeIsMacro[] = {
   /* LinearGeneral = */   false,
   /* LinearAligned = */   false,
   /* Tiled1DThin1 = */    false,
   /* Tiled1DThick = */    false,
   /* Tiled2DThin1 = */    true,
   /* Tiled2DThin2 = */    true,
   /* Tiled2DThin4 = */    true,
   /* Tiled2DThick = */    true,
   /* Tiled2BThin1 = */    true,
   /* Tiled2BThin2 = */    true,
   /* Tiled2BThin4 = */    true,
   /* Tiled2BThick = */    true,
   /* Tiled3DThin1 = */    true,
   /* Tiled3DThick = */    true,
   /* Tiled3BThin1 = */    true,
   /* Tiled3BThick = */    true,
};

static constexpr bool TileModeIsMacro3X[] = {
   /* LinearGeneral = */   false,
   /* LinearAligned = */   false,
   /* Tiled1DThin1 = */    false,
   /* Tiled1DThick = */    false,
   /* Tiled2DThin1 = */    false,
   /* Tiled2DThin2 = */    false,
   /* Tiled2DThin4 = */    false,
   /* Tiled2DThick = */    false,
   /* Tiled2BThin1 = */    false,
   /* Tiled2BThin2 = */    false,
   /* Tiled2BThin4 = */    false,
   /* Tiled2BThick = */    false,
   /* Tiled3DThin1 = */    true,
   /* Tiled3DThick = */    true,
   /* Tiled3BThin1 = */    true,
   /* Tiled3BThick = */    true,
};

static constexpr bool TileModeIsBankSwapped[] = {
   /* LinearGeneral = */   false,
   /* LinearAligned = */   false,
   /* Tiled1DThin1 = */    false,
   /* Tiled1DThick = */    false,
   /* Tiled2DThin1 = */    false,
   /* Tiled2DThin2 = */    false,
   /* Tiled2DThin4 = */    false,
   /* Tiled2DThick = */    false,
   /* Tiled2BThin1 = */    true,
   /* Tiled2BThin2 = */    true,
   /* Tiled2BThin4 = */    true,
   /* Tiled2BThick = */    true,
   /* Tiled3DThin1 = */    false,
   /* Tiled3DThick = */    false,
   /* Tiled3BThin1 = */    true,
   /* Tiled3BThick = */    true,
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

static constexpr int
getTileModeIsMacro(TileMode tileMode)
{
   return TileModeIsMacro[static_cast<size_t>(tileMode)];
}

static constexpr int
getTileModeIs3X(TileMode tileMode)
{
   return TileModeIsMacro3X[static_cast<size_t>(tileMode)];
}

static constexpr int
getTileModeIsBankSwapped(TileMode tileMode)
{
   return TileModeIsBankSwapped[static_cast<size_t>(tileMode)];
}

struct SurfaceDescription
{
   TileMode tileMode = TileMode::LinearGeneral;
   DataFormat format = DataFormat::Invalid;
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

struct SurfaceInfo
{
   TileMode tileMode;
   SurfaceUse use;
   uint32_t bpp;

   uint32_t pitch;
   uint32_t height;
   uint32_t depth;
   uint32_t surfSize;
   uint32_t sliceSize;
   uint32_t baseAlign;
   uint32_t pitchAlign;
   uint32_t heightAlign;
   uint32_t depthAlign;
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
};

struct RetileInfo
{
   // Input values
   TileMode tileMode;
   uint32_t bitsPerElement;
   bool isDepth;

   // Some helpful stuff
   uint32_t thinSliceBytes;

   // Used for both micro and macro tiling
   bool isTiled;
   bool isMacroTiled;
   uint32_t macroTileWidth;
   uint32_t macroTileHeight;
   uint32_t microTileThickness;
   uint32_t thickMicroTileBytes;
   uint32_t numTilesPerRow;
   uint32_t numTilesPerSlice;

   // Used only for macro tiling
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
   uint32_t bankSwapWidth;
};

SurfaceInfo
computeSurfaceInfo(const SurfaceDescription &surface,
                   int mipLevel);

size_t
computeUnpitchedImageSize(const SurfaceDescription &desc);

size_t
computeUnpitchedMipMapSize(const SurfaceDescription &desc);

void
unpitchImage(const SurfaceDescription &desc,
             const void *pitched,
             void *unpitched);

void
unpitchMipMap(const SurfaceDescription &desc,
              const void *pitched,
              void *unpitched);

RetileInfo
computeRetileInfo(const SurfaceInfo &info);

} // namespace gpu7::tiling
