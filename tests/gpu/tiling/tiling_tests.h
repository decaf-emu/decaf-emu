#pragma once

#include <addrlib/addrinterface.h>
#include <catch.hpp>
#include <random>
#include <spdlog/spdlog.h>
#include <vector>

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
   AddrFormat format;
   uint32_t bpp;
   bool depth;
};

struct TestTilingMode
{
   AddrTileMode tileMode;
};

static constexpr TestLayout sTestLayout[] = {
   { 1u, 1u, 1u, 0u, 1u },
   { 338u, 309u, 1u, 0u, 1u },
   { 338u, 309u, 11u, 2u, 5u},

   // The variants above cover these, but they are useful for
   // debugging various errors in the algorithms.  The matrix
   // is already huge though, so disabling by default.
   //{ 338u, 309u, 4u, 0u, 4u },
};

static constexpr TestLayout sPerfTestLayout = { 338u, 309u, 8u, 0u, 8u };

static constexpr TestTilingMode sTestTilingMode[] = {
   { ADDR_TM_1D_TILED_THIN1 },
   { ADDR_TM_1D_TILED_THICK },
   { ADDR_TM_2D_TILED_THIN1 },
   { ADDR_TM_2D_TILED_THIN2 },
   { ADDR_TM_2D_TILED_THIN4 },
   { ADDR_TM_2D_TILED_THICK },
   { ADDR_TM_2B_TILED_THIN1 },
   { ADDR_TM_2B_TILED_THIN2 },
   { ADDR_TM_2B_TILED_THIN4 },
   { ADDR_TM_2B_TILED_THICK },
   { ADDR_TM_3D_TILED_THIN1 },
   { ADDR_TM_3D_TILED_THICK },
   { ADDR_TM_3B_TILED_THIN1 },
   { ADDR_TM_3B_TILED_THICK },
};

static constexpr TestFormat sTestFormats[] = {
   { AddrFormat::ADDR_FMT_8, 8u, false },
   { AddrFormat::ADDR_FMT_8_8, 16u, false },
   { AddrFormat::ADDR_FMT_8_8_8_8, 32u, false },
   { AddrFormat::ADDR_FMT_32_32, 64u, false },
   { AddrFormat::ADDR_FMT_32_32_32_32, 128u, false },
   //{ AddrFormat::ADDR_FMT_16, 16u, true },
   //{ AddrFormat::ADDR_FMT_32, 32u, true },
   //{ AddrFormat::ADDR_FMT_X24_8_32_FLOAT, 64u, true },
};

static const char *
tileModeToString(AddrTileMode mode)
{
   switch (mode) {
   case ADDR_TM_LINEAR_GENERAL:
      return "LinearGeneral";
   case ADDR_TM_LINEAR_ALIGNED:
      return "LinearAligned";
   case ADDR_TM_1D_TILED_THIN1:
      return "Tiled1DThin1";
   case ADDR_TM_1D_TILED_THICK:
      return "Tiled1DThick";
   case ADDR_TM_2D_TILED_THIN1:
      return "Tiled2DThin1";
   case ADDR_TM_2D_TILED_THIN2:
      return "Tiled2DThin2";
   case ADDR_TM_2D_TILED_THIN4:
      return "Tiled2DThin4";
   case ADDR_TM_2D_TILED_THICK:
      return "Tiled2DThick";
   case ADDR_TM_2B_TILED_THIN1:
      return "Tiled2BThin1";
   case ADDR_TM_2B_TILED_THIN2:
      return "Tiled2BThin2";
   case ADDR_TM_2B_TILED_THIN4:
      return "Tiled2BThin4";
   case ADDR_TM_2B_TILED_THICK:
      return "Tiled2BThick";
   case ADDR_TM_3D_TILED_THIN1:
      return "Tiled3DThin1";
   case ADDR_TM_3D_TILED_THICK:
      return "Tiled3DThick";
   case ADDR_TM_3B_TILED_THIN1:
      return "Tiled3BThin1";
   case ADDR_TM_3B_TILED_THICK:
      return "Tiled3BThick";
   default:
      FAIL(fmt::format("Unknown tiling mode {}", static_cast<int>(mode)));
      return "Unknown";
   }
}
