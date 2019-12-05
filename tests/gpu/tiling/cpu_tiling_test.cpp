#include "tiling_tests.h"
#include "addrlib_helpers.h"
#include "test_helpers.h"

#include <common/align.h>
#include <libgpu/gpu7_tiling_cpu.h>

static inline void
compareTilingToAddrLib(const gpu7::tiling::SurfaceDescription& desc,
                       std::vector<uint8_t>& input,
                       uint32_t firstSlice,
                       uint32_t numSlices)
{
   auto addrLib = AddrLib { };

   auto alibUntiled = std::vector<uint8_t> { };
   auto gpu7Untiled = std::vector<uint8_t> { };
   auto alibTiled = std::vector<uint8_t> { };
   auto gpu7Tiled = std::vector<uint8_t> { };

   // Compute surface info
   auto alibInfo = addrLib.computeSurfaceInfo(desc, 0, 0);
   auto gpu7Info = gpu7::tiling::computeSurfaceInfo(desc, 0);

   REQUIRE(gpu7Info.surfSize == alibInfo.surfSize);
   REQUIRE(input.size() >= gpu7Info.surfSize);

   alibUntiled.resize(alibInfo.surfSize);
   alibTiled.resize(alibInfo.surfSize);
   gpu7Untiled.resize(gpu7Info.surfSize);
   gpu7Tiled.resize(gpu7Info.surfSize);

   // AddrLib
   {
      addrLib.untileSlices(desc, 0, input.data(), alibUntiled.data(), firstSlice, numSlices);
      addrLib.tileSlices(desc, 0, alibUntiled.data(), alibTiled.data(), firstSlice, numSlices);
   }

   // GPU7
   {
      auto retileInfo = gpu7::tiling::computeRetileInfo(gpu7Info);

      auto tiledFirstSliceIndex = align_down(firstSlice, retileInfo.microTileThickness);
      auto tiledSliceOffset = tiledFirstSliceIndex * retileInfo.thinSliceBytes;
      auto untiledSliceOffset = firstSlice * retileInfo.thinSliceBytes;

      gpu7::tiling::cpu::untile(retileInfo,
                                gpu7Untiled.data() + untiledSliceOffset,
                                input.data() + tiledSliceOffset,
                                firstSlice, numSlices);

      gpu7::tiling::cpu::tile(retileInfo,
                              gpu7Untiled.data() + untiledSliceOffset,
                              gpu7Tiled.data() + tiledSliceOffset,
                              firstSlice, numSlices);
   }

   CHECK(compareImages(gpu7Untiled, alibUntiled));
   CHECK(compareImages(gpu7Tiled, alibTiled));
}

TEST_CASE("cpuTiling")
{
   for (auto& layout : sTestLayout) {
      SECTION(fmt::format("{}x{}x{} s{}n{}",
                          layout.width, layout.height, layout.depth,
                          layout.testFirstSlice, layout.testNumSlices))
      {
         for (auto& mode : sTestTilingMode) {
            SECTION(fmt::format("{}", tileModeToString(mode.tileMode)))
            {
               for (auto& format : sTestFormats) {
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
                     surface.numLevels = 1u;
                     surface.bankSwizzle = 0u;
                     surface.pipeSwizzle = 0u;
                     surface.dim = gpu7::tiling::SurfaceDim::Texture2DArray;
                     surface.use = format.depth ?
                        gpu7::tiling::SurfaceUse::DepthBuffer :
                        gpu7::tiling::SurfaceUse::None;

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

struct ALibPendingCpuPerfEntry
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
   auto& untiled = sRandomData;

   // Some place to store pending tests
   std::vector<ALibPendingCpuPerfEntry> pendingTests;

   // Generate all the test cases to run
   auto& layout = sPerfTestLayout;
   for (auto& mode : sTestTilingMode) {
      for (auto& format : sTestFormats) {
         auto surface = gpu7::tiling::SurfaceDescription {};
         surface.tileMode = mode.tileMode;
         surface.format = format.format;
         surface.bpp = format.bpp;
         surface.width = layout.width;
         surface.height = layout.height;
         surface.numSlices = layout.depth;
         surface.numSamples = 1u;
         surface.numLevels = 1u;
         surface.bankSwizzle = 0u;
         surface.pipeSwizzle = 0u;
         surface.dim = gpu7::tiling::SurfaceDim::Texture2DArray;
         surface.use = format.depth ?
            gpu7::tiling::SurfaceUse::DepthBuffer :
            gpu7::tiling::SurfaceUse::None;

         auto info = addrLib.computeSurfaceInfo(surface, 0, 0);

         ALibPendingCpuPerfEntry test;
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
      for (auto& test : pendingTests) {
         // Compare image
         addrLib.untileSlices(test.desc, 0,
                              untiled.data(), addrLibImage.data(),
                              test.firstSlice, test.numSlices);
      }
   }
}

struct PendingCpuPerfEntry
{
   gpu7::tiling::SurfaceDescription desc;
   uint32_t firstSlice;
   uint32_t numSlices;

   gpu7::tiling::SurfaceInfo info;
};

TEST_CASE("cpuTilingPerf", "[!benchmark]")
{
   // Set up AddrLib to generate data to test against
   auto addrLib = AddrLib { };

   // Get some random data to use
   auto& untiled = sRandomData;

   // Some place to store pending tests
   std::vector<PendingCpuPerfEntry> pendingTests;

   // Generate all the test cases to run
   auto& layout = sPerfTestLayout;
   for (auto& mode : sTestTilingMode) {
      for (auto& format : sTestFormats) {
         auto surface = gpu7::tiling::SurfaceDescription {};
         surface.tileMode = mode.tileMode;
         surface.format = format.format;
         surface.bpp = format.bpp;
         surface.width = layout.width;
         surface.height = layout.height;
         surface.numSlices = layout.depth;
         surface.numSamples = 1u;
         surface.numLevels = 1u;
         surface.bankSwizzle = 0u;
         surface.pipeSwizzle = 0u;
         surface.dim = gpu7::tiling::SurfaceDim::Texture2DArray;
         surface.use = format.depth ?
            gpu7::tiling::SurfaceUse::DepthBuffer :
            gpu7::tiling::SurfaceUse::None;

         auto info = gpu7::tiling::computeSurfaceInfo(surface, 0);

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

   static constexpr auto TestIterMulti = 10;

   auto benchTitle = fmt::format("processing ({} retiles)", pendingTests.size() * TestIterMulti);
   BENCHMARK(benchTitle)
   {
      for (auto i = 0; i < TestIterMulti; ++i) {
         for (auto& test : pendingTests) {

            auto retileInfo = gpu7::tiling::computeRetileInfo(test.info);

            auto tiledFirstSliceIndex = align_down(test.firstSlice, retileInfo.microTileThickness);
            auto tiledSliceOffset = tiledFirstSliceIndex * retileInfo.thinSliceBytes;
            auto untiledSliceOffset = test.firstSlice * retileInfo.thinSliceBytes;

            gpu7::tiling::cpu::untile(retileInfo,
                                      untiled.data() + untiledSliceOffset,
                                      tiledImage.data() + tiledSliceOffset,
                                      test.firstSlice,
                                      test.numSlices);
         }
      }
   }
}

