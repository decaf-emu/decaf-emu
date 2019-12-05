#pragma once

#include <addrlib/addrinterface.h>
#include <catch.hpp>
#include <random>
#include <spdlog/spdlog.h>
#include <vector>
#include <libgpu/gpu7_tiling.h>

extern std::vector<uint8_t> sRandomData;

struct TestLayout
{
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t testFirstSlice;
   uint32_t testNumSlices;
};

struct TestFormat
{
   gpu7::tiling::DataFormat format;
   uint32_t bpp;
   bool depth;
};

struct TestTilingMode
{
   gpu7::tiling::TileMode tileMode;
};

static constexpr TestLayout sTestLayout[] = {
   { 1u, 1u, 1u, 0u, 1u },
   { 1u, 1u, 11u, 5u, 5u },
   { 338u, 309u, 1u, 0u, 1u },
   { 338u, 309u, 11u, 5u, 5u },

   // The variants above cover these, but they are useful for
   // debugging various errors in the algorithms.  The matrix
   // is already huge though, so disabling by default.
   //{ 338u, 309u, 4u, 0u, 4u },
};

static constexpr TestLayout sPerfTestLayout = { 338u, 309u, 8u, 0u, 8u };

static constexpr TestTilingMode sTestTilingMode[] = {
   { gpu7::tiling::TileMode::Micro1DTiledThin1 },
   { gpu7::tiling::TileMode::Micro1DTiledThick },
   { gpu7::tiling::TileMode::Macro2DTiledThin1 },
   { gpu7::tiling::TileMode::Macro2DTiledThin2 },
   { gpu7::tiling::TileMode::Macro2DTiledThin4 },
   { gpu7::tiling::TileMode::Macro2DTiledThick },
   { gpu7::tiling::TileMode::Macro2BTiledThin1 },
   { gpu7::tiling::TileMode::Macro2BTiledThin2 },
   { gpu7::tiling::TileMode::Macro2BTiledThin4 },
   { gpu7::tiling::TileMode::Macro2BTiledThick },
   { gpu7::tiling::TileMode::Macro3DTiledThin1 },
   { gpu7::tiling::TileMode::Macro3DTiledThick },
   { gpu7::tiling::TileMode::Macro3BTiledThin1 },
   { gpu7::tiling::TileMode::Macro3BTiledThick },
};

static constexpr TestFormat sTestFormats[] = {
   { gpu7::tiling::DataFormat::FMT_8, 8u, false },
   { gpu7::tiling::DataFormat::FMT_8_8, 16u, false },
   { gpu7::tiling::DataFormat::FMT_8_8_8_8, 32u, false },
   { gpu7::tiling::DataFormat::FMT_32_32, 64u, false },
   { gpu7::tiling::DataFormat::FMT_32_32_32_32, 128u, false },
   //{ gpu7::tiling::DataFormat::FMT_16, 16u, true },
   //{ gpu7::tiling::DataFormat::FMT_32, 32u, true },
   //{ gpu7::tiling::DataFormat::FMT_X24_8_32_FLOAT, 64u, true },
};

static const char*
tileModeToString(gpu7::tiling::TileMode mode)
{
   switch (mode) {
   case gpu7::tiling::TileMode::LinearGeneral:
      return "LinearGeneral";
   case gpu7::tiling::TileMode::LinearAligned:
      return "LinearAligned";
   case gpu7::tiling::TileMode::Micro1DTiledThin1:
      return "Tiled1DThin1";
   case gpu7::tiling::TileMode::Micro1DTiledThick:
      return "Tiled1DThick";
   case gpu7::tiling::TileMode::Macro2DTiledThin1:
      return "Tiled2DThin1";
   case gpu7::tiling::TileMode::Macro2DTiledThin2:
      return "Tiled2DThin2";
   case gpu7::tiling::TileMode::Macro2DTiledThin4:
      return "Tiled2DThin4";
   case gpu7::tiling::TileMode::Macro2DTiledThick:
      return "Tiled2DThick";
   case gpu7::tiling::TileMode::Macro2BTiledThin1:
      return "Tiled2BThin1";
   case gpu7::tiling::TileMode::Macro2BTiledThin2:
      return "Tiled2BThin2";
   case gpu7::tiling::TileMode::Macro2BTiledThin4:
      return "Tiled2BThin4";
   case gpu7::tiling::TileMode::Macro2BTiledThick:
      return "Tiled2BThick";
   case gpu7::tiling::TileMode::Macro3DTiledThin1:
      return "Tiled3DThin1";
   case gpu7::tiling::TileMode::Macro3DTiledThick:
      return "Tiled3DThick";
   case gpu7::tiling::TileMode::Macro3BTiledThin1:
      return "Tiled3BThin1";
   case gpu7::tiling::TileMode::Macro3BTiledThick:
      return "Tiled3BThick";
   default:
      FAIL(fmt::format("Unknown tiling mode {}", static_cast<int>(mode)));
      return "Unknown";
   }
}
