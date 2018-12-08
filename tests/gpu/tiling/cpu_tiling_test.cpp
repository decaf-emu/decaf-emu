#include "tiling_tests.h"
#include "addrlib_helpers.h"
#include "test_helpers.h"

#include <common/align.h>
#include <libgpu/gpu7_tiling.h>

static inline void
compareTilingToAddrLib(const gpu7::tiling::SurfaceDescription &desc,
                       std::vector<uint8_t> &untiled,
                       uint32_t firstSlice, uint32_t numSlices)
{
   auto addrLib = AddrLib { };

   // Compute surface info
   auto info = addrLib.computeSurfaceInfo(desc, 0, 0);

   // Ensure our untiled random data is large enough
   REQUIRE(untiled.size() >= info.surfSize);

   // Compare image
   auto addrLibImage = std::vector<uint8_t> { };
   addrLibImage.resize(info.surfSize);
   addrLib.untileSlices(desc, 0, untiled.data(), addrLibImage.data(), firstSlice, numSlices);

   auto tiledImage = std::vector<uint8_t> { };
   tiledImage.resize(info.surfSize);
   for (auto sliceIdx = firstSlice; sliceIdx < firstSlice + numSlices; ++sliceIdx) {
      auto sliceOffset = info.sliceSize * sliceIdx;

      gpu7::tiling::untileImageSlice(desc,
                                     untiled.data(),
                                     tiledImage.data() + sliceOffset,
                                     sliceIdx);
   }

   CHECK(compareImages(tiledImage, addrLibImage));

   return;

   // Compare mipmaps
   auto addrLibMipMap = std::vector<uint8_t> { };

   for (auto level = 1u; level < desc.numLevels; ++level) {
      auto mipInfo = addrLib.computeSurfaceInfo(desc, 0, level);
      auto offset = align_up(addrLibMipMap.size(), mipInfo.baseAlign);
      addrLibMipMap.resize(offset + mipInfo.surfSize);

      addrLib.untileSlices(desc, level,
                           untiled.data() + offset,
                           addrLibMipMap.data() + offset,
                           firstSlice, numSlices);
   }

   auto tiledMipMap = std::vector<uint8_t> { };
   tiledMipMap.resize(addrLibMipMap.size());

   auto mipOffset = size_t { 0 };
   for (auto level = 1u; level < desc.numLevels; ++level) {
      auto info = computeSurfaceInfo(desc, level, 0);
      mipOffset = align_up(mipOffset, info.baseAlign);

      for (auto sliceIdx = firstSlice; sliceIdx < firstSlice + numSlices; ++sliceIdx) {
         auto sliceOffset = info.sliceSize * sliceIdx;

         gpu7::tiling::untileMipSlice(desc,
                                      untiled.data() + mipOffset,
                                      tiledMipMap.data() + mipOffset + sliceOffset,
                                      level,
                                      sliceIdx);
      }

      mipOffset += info.surfSize;
   }

   CHECK(compareImages(tiledMipMap, addrLibMipMap));
}

TEST_CASE("cpuTiling")
{
   for (auto &layout : sTestLayout) {
      SECTION(fmt::format("{}x{}x{} s{}n{}",
                          layout.width, layout.height, layout.depth,
                          layout.testFirstSlice, layout.testNumSlices))
      {
         for (auto &mode : sTestTilingMode) {
            SECTION(fmt::format("{}", tileModeToString(mode.tileMode)))
            {
               for (auto &format : sTestFormats) {
                  SECTION(fmt::format("{}bpp{}", format.bpp, format.depth ? " depth" : ""))
                  {
                     auto surface = gpu7::tiling::SurfaceDescription { };
                     surface.tileMode = mode.tileMode;
                     surface.format = format.format;
                     surface.bpp = format.bpp;
                     surface.width = layout.width;
                     surface.height = layout.height;
                     surface.numSlices = layout.depth;
                     surface.numSamples = 1u;
                     surface.numLevels = 9u;
                     surface.bankSwizzle = 0u;
                     surface.pipeSwizzle = 0u;
                     surface.flags.inputBaseMap = 1;
                     surface.flags.depth = format.depth ? 1 : 0;
                     compareTilingToAddrLib(surface,
                                            sRandomData,
                                            layout.testFirstSlice,
                                            layout.testNumSlices);
                  }
               }
            }
         }
      }
   }
}

struct PendingCpuPerfEntry
{
   gpu7::tiling::SurfaceDescription desc;
   uint32_t firstSlice;
   uint32_t numSlices;

   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
};

TEST_CASE("alibTilingPerf", "[!benchmark]")
{
   // Set up AddrLib to generate data to test against
   auto addrLib = AddrLib { };

   // Get some random data to use
   auto &untiled = sRandomData;

   // Some place to store pending tests
   std::vector<PendingCpuPerfEntry> pendingTests;

   // Generate all the test cases to run
   auto &layout = sPerfTestLayout;
   for (auto &mode : sTestTilingMode) {
      for (auto &format : sTestFormats) {
         auto surface = gpu7::tiling::SurfaceDescription {};
         surface.tileMode = mode.tileMode;
         surface.format = format.format;
         surface.bpp = format.bpp;
         surface.width = layout.width;
         surface.height = layout.height;
         surface.numSlices = layout.depth;
         surface.numSamples = 1u;
         surface.numLevels = 9u;
         surface.bankSwizzle = 0u;
         surface.pipeSwizzle = 0u;
         surface.flags.inputBaseMap = 1;
         surface.flags.depth = format.depth ? 1 : 0;

         auto info = addrLib.computeSurfaceInfo(surface, 0, 0);

         PendingCpuPerfEntry test;
         test.desc = surface;
         test.info = info;
         test.firstSlice = layout.testFirstSlice;
         test.numSlices = layout.testNumSlices;
         pendingTests.push_back(test);
      }
   }

   auto addrLibImage = std::vector<uint8_t> { };
   addrLibImage.resize(untiled.size());

   auto benchTitle = fmt::format("processing ({} retiles)", pendingTests.size());
   BENCHMARK(benchTitle)
   {
      for (auto &test : pendingTests) {
         // Compare image
         addrLib.untileSlices(test.desc, 0,
                              untiled.data(), addrLibImage.data(),
                              test.firstSlice, test.numSlices);
      }
   }
}

TEST_CASE("cpuTilingPerf", "[!benchmark]")
{
   // Set up AddrLib to generate data to test against
   auto addrLib = AddrLib { };

   // Get some random data to use
   auto &untiled = sRandomData;

   // Some place to store pending tests
   std::vector<PendingCpuPerfEntry> pendingTests;

   // Generate all the test cases to run
   auto &layout = sPerfTestLayout;
   for (auto &mode : sTestTilingMode) {
      for (auto &format : sTestFormats) {
         auto surface = gpu7::tiling::SurfaceDescription {};
         surface.tileMode = mode.tileMode;
         surface.format = format.format;
         surface.bpp = format.bpp;
         surface.width = layout.width;
         surface.height = layout.height;
         surface.numSlices = layout.depth;
         surface.numSamples = 1u;
         surface.numLevels = 9u;
         surface.bankSwizzle = 0u;
         surface.pipeSwizzle = 0u;
         surface.flags.inputBaseMap = 1;
         surface.flags.depth = format.depth ? 1 : 0;

         auto info = addrLib.computeSurfaceInfo(surface, 0, 0);

         PendingCpuPerfEntry test;
         test.desc = surface;
         test.info = info;
         test.firstSlice = layout.testFirstSlice;
         test.numSlices = layout.testNumSlices;
         pendingTests.push_back(test);
      }
   }

   auto tiledImage = std::vector<uint8_t> { };
   tiledImage.resize(untiled.size());

   auto benchTitle = fmt::format("processing ({} retiles)", pendingTests.size());
   BENCHMARK(benchTitle)
   {
      for (auto &test : pendingTests) {
         auto firstSlice = test.firstSlice;
         auto lastSlice = test.firstSlice + test.numSlices;
         for (auto sliceIdx = firstSlice; sliceIdx < lastSlice; ++sliceIdx) {
            auto sliceOffset = test.info.sliceSize * sliceIdx;
            gpu7::tiling::untileImageSlice(test.desc,
                                           untiled.data(),
                                           tiledImage.data() + sliceOffset,
                                           sliceIdx);
         }
      }
   }
}

