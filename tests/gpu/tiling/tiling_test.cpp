#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <addrlib/addrinterface.h>
#include <cassert>
#include <cstdlib>
#include <common/align.h>
#include <common/log.h>
#include <fmt/format.h>
#include <libgpu/gpu7_tiling.h>
#include <random>

std::shared_ptr<spdlog::logger>
gLog;

class AddrLib
{
public:
   AddrLib()
   {
      ADDR_CREATE_INPUT input;
      ADDR_CREATE_OUTPUT output;
      std::memset(&input, 0, sizeof(input));
      std::memset(&output, 0, sizeof(output));

      input.size = sizeof(ADDR_CREATE_INPUT);
      output.size = sizeof(ADDR_CREATE_OUTPUT);

      input.chipEngine = CIASICIDGFXENGINE_R600;
      input.chipFamily = 0x51;
      input.chipRevision = 71;
      input.createFlags.fillSizeFields = 1;
      input.regValue.gbAddrConfig = 0x44902;

      input.callbacks.allocSysMem = &addrLibAlloc;
      input.callbacks.freeSysMem = &addrLibFree;

      REQUIRE(AddrCreate(&input, &output) == ADDR_OK);
      mHandle = output.hLib;
   }

   ~AddrLib()
   {
      if (mHandle) {
         AddrDestroy(mHandle);
      }
   }

   void
   untile(const gpu7::tiling::SurfaceDescription &desc,
          uint32_t mipLevel,
          const uint8_t *src,
          uint8_t *dst)
   {
      for (uint32_t sample = 0; sample < desc.numSamples; ++sample) {
         for (uint32_t slice = 0; slice < desc.numSlices; ++slice) {
            const auto info = computeSurfaceInfo(desc, slice, mipLevel);
            auto srcAddrInput = getTiledAddrFromCoordInput(desc, info);
            srcAddrInput.sample = sample;
            srcAddrInput.slice = slice;

            auto dstAddrInput = getUntiledAddrFromCoordInput(desc, info);
            dstAddrInput.sample = sample;
            dstAddrInput.slice = slice;

            copySurfacePixels(src, srcAddrInput, dst, dstAddrInput);
         }
      }
   }

   void
   tile(const gpu7::tiling::SurfaceDescription &desc,
        uint32_t mipLevel,
        uint8_t *src,
        uint8_t *dst)
   {
      for (uint32_t sample = 0; sample < desc.numSamples; ++sample) {
         for (uint32_t slice = 0; slice < desc.numSlices; ++slice) {
            const auto info = computeSurfaceInfo(desc, 0, mipLevel);

            auto srcAddrInput = getUntiledAddrFromCoordInput(desc, info);
            srcAddrInput.sample = sample;
            srcAddrInput.slice = slice;

            auto dstAddrInput = getTiledAddrFromCoordInput(desc, info);
            dstAddrInput.sample = sample;
            dstAddrInput.slice = slice;

            copySurfacePixels(src, srcAddrInput, dst, dstAddrInput);
         }
      }
   }

   ADDR_COMPUTE_SURFACE_INFO_OUTPUT
   computeSurfaceInfo(const gpu7::tiling::SurfaceDescription &desc,
                      uint32_t slice,
                      uint32_t mipLevel)
   {
      auto output = ADDR_COMPUTE_SURFACE_INFO_OUTPUT { };
      output.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT);

      auto input = ADDR_COMPUTE_SURFACE_INFO_INPUT { };
      input.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT);
      input.tileMode = desc.tileMode;
      input.format = desc.format;
      input.bpp = desc.bpp;
      input.numSamples = desc.numSamples;
      input.width = desc.width;
      input.height = desc.height;
      input.numSlices = desc.numSlices;
      input.flags = desc.flags;
      input.numFrags = desc.numFrags;
      input.slice = slice;
      input.mipLevel = mipLevel;
      REQUIRE(AddrComputeSurfaceInfo(mHandle, &input, &output) == ADDR_OK);
      return output;
   }

private:
   static void *
   addrLibAlloc(const ADDR_ALLOCSYSMEM_INPUT *pInput)
   {
      return std::malloc(pInput->sizeInBytes);
   }

   static ADDR_E_RETURNCODE
   addrLibFree(const ADDR_FREESYSMEM_INPUT *pInput)
   {
      std::free(pInput->pVirtAddr);
      return ADDR_OK;
   }

   void
   copySurfacePixels(const uint8_t *src,
                     ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput,
                     uint8_t *dst,
                     ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput)
   {
      auto srcAddrOutput = ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT { };
      auto dstAddrOutput = ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT { };
      srcAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);
      dstAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);

      assert(srcAddrInput.bpp == dstAddrInput.bpp);
      assert(srcAddrInput.pitch == dstAddrInput.pitch);
      assert(srcAddrInput.height == dstAddrInput.height);
      auto bytesPerElem = dstAddrInput.bpp / 8;

      for (auto y = 0u; y < dstAddrInput.height; ++y) {
         for (auto x = 0u; x < dstAddrInput.pitch; ++x) {
            dstAddrInput.x = x;
            dstAddrInput.y = y;
            AddrComputeSurfaceAddrFromCoord(mHandle, &dstAddrInput, &dstAddrOutput);

            srcAddrInput.x = x;
            srcAddrInput.y = y;
            AddrComputeSurfaceAddrFromCoord(mHandle, &srcAddrInput, &srcAddrOutput);

            std::memcpy(dst + dstAddrOutput.addr,
                        src + srcAddrOutput.addr,
                        bytesPerElem);
         }
      }
   }

   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT
   getUntiledAddrFromCoordInput(const gpu7::tiling::SurfaceDescription &desc,
                                const ADDR_COMPUTE_SURFACE_INFO_OUTPUT &info)
   {
      auto input = getTiledAddrFromCoordInput(desc, info);
      input.tileMode = AddrTileMode::ADDR_TM_LINEAR_GENERAL;
      input.bankSwizzle = 0;
      input.pipeSwizzle = 0;
      return input;
   }

   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT
   getTiledAddrFromCoordInput(const gpu7::tiling::SurfaceDescription &desc,
                              const ADDR_COMPUTE_SURFACE_INFO_OUTPUT &info)
   {
      auto input = ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT { };
      input.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
      input.bpp = info.bpp;
      input.pitch = info.pitch;
      input.height = info.height;
      input.numSlices = info.depth;
      input.numSamples = desc.numSamples;
      input.tileMode = info.tileMode;
      input.isDepth = !!desc.flags.depth;
      input.tileBase = 0;
      input.compBits = 0;
      input.numFrags = desc.numFrags;
      input.bankSwizzle = desc.bankSwizzle;
      input.pipeSwizzle = desc.pipeSwizzle;
      return input;
   }

private:
   ADDR_HANDLE mHandle = nullptr;
};

static std::vector<uint8_t>
generateRandomData(size_t size)
{
   std::mt19937 eng { 0x0DECAF10 };
   std::uniform_int_distribution<uint32_t> urd { 0, 255 };
   std::vector<uint8_t> result;
   result.resize(size);

   for (auto i = 0; i < size; ++i) {
      result[i] = static_cast<uint8_t>(urd(eng));
   }

   return result;
}

static bool
compareImages(const std::vector<uint8_t> &data,
              const std::vector<uint8_t> &reference)
{
   REQUIRE(data.size() == reference.size());

   for (auto i = 0u; i < data.size(); ++i) {
      if (data[i] != reference[i]) {
         WARN(fmt::format("Difference at offset {}, 0x{:02X} != 0x{:02X}", i, data[i], reference[i]));
         return false;
      }
   }

   return true;
}

void
compareTilingToAddrLib(const gpu7::tiling::SurfaceDescription &desc,
                       std::vector<uint8_t> &untiled)
{
   auto addrLib = AddrLib { };

   // Compute surface info
   auto info = addrLib.computeSurfaceInfo(desc, 0, 0);

   // Ensure our untiled random data is large enough
   REQUIRE(untiled.size() >= info.surfSize);

   // Compare image
   auto addrLibImage = std::vector<uint8_t> { };
   addrLibImage.resize(info.surfSize);
   addrLib.untile(desc, 0, untiled.data(), addrLibImage.data());

   auto tiledImage = std::vector<uint8_t> { };
   tiledImage.resize(info.surfSize);
   gpu7::tiling::untileImage(desc, untiled.data(), tiledImage.data());

   CHECK(compareImages(tiledImage, addrLibImage));

   // Compare mipmaps
   auto addrLibMipMap = std::vector<uint8_t> { };

   for (auto level = 1u; level < desc.numLevels; ++level) {
      auto mipInfo = addrLib.computeSurfaceInfo(desc, 0, level);
      auto offset = align_up(addrLibMipMap.size(), mipInfo.baseAlign);
      addrLibMipMap.resize(offset + mipInfo.surfSize);

      addrLib.untile(desc, level,
                     untiled.data() + offset,
                     addrLibMipMap.data() + offset);
   }

   auto tiledMipMap = std::vector<uint8_t> { };
   tiledMipMap.resize(addrLibMipMap.size());
   gpu7::tiling::untileMipMap(desc, untiled.data(), tiledMipMap.data());

   CHECK(compareImages(tiledMipMap, addrLibMipMap));
}

struct TestFormat
{
   AddrFormat format;
   uint32_t bpp;
};

struct TestTilingMode
{
   AddrTileMode tileMode;
};

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
   { AddrFormat::ADDR_FMT_8, 8u },
   { AddrFormat::ADDR_FMT_8_8, 16u },
   { AddrFormat::ADDR_FMT_8_8_8_8, 32u },
   { AddrFormat::ADDR_FMT_32_32, 64u },
   { AddrFormat::ADDR_FMT_32_32_32_32, 128u },
};

static auto sRandomData = generateRandomData(32 * 1024 * 1024);

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

TEST_CASE("cpuTiling")
{
   for (auto &mode : sTestTilingMode) {
      SECTION(fmt::format("{}", tileModeToString(mode.tileMode)))
      {
         for (auto &format : sTestFormats) {
            SECTION(fmt::format("{}bpp", format.bpp))
            {
               auto surface = gpu7::tiling::SurfaceDescription { };
               surface.tileMode = mode.tileMode;
               surface.format = format.format;
               surface.bpp = format.bpp;
               surface.width = 338u;
               surface.height = 309u;
               surface.numSlices = 1u;
               surface.numSamples = 1u;
               surface.numLevels = 9u;
               surface.bankSwizzle = 0u;
               surface.pipeSwizzle = 0u;
               surface.flags.inputBaseMap = 1;
               compareTilingToAddrLib(surface, sRandomData);
            }
         }
      }
   }
}
