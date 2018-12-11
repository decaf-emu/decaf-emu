#ifdef DECAF_VULKAN

#include "tiling_tests.h"
#include "addrlib_helpers.h"
#include "test_helpers.h"

#include <common/align.h>
#include <libgpu/gpu7_tiling_vulkan.h>
#include <vector>

#include "vulkan_helpers.h"

static gpu7::tiling::vulkan::Retiler gVkRetiler;

static inline void
compareTilingToAddrLib(const gpu7::tiling::SurfaceDescription &desc,
                       std::vector<uint8_t> &untiled,
                       uint32_t firstSlice, uint32_t numSlices)
{
   // Set up AddrLib to generate data to test against
   auto addrLib = AddrLib { };

   // Compute some needed surface information
   auto info = addrLib.computeSurfaceInfo(desc, 0, 0);
   auto surfSize = static_cast<uint32_t>(info.surfSize);

   // Make sure our test data is big enough
   REQUIRE(untiled.size() >= info.surfSize);

   // Generate the correct version of the data
   auto addrLibImage = std::vector<uint8_t> { };
   addrLibImage.resize(info.surfSize);
   addrLib.untileSlices(desc, 0, untiled.data(), addrLibImage.data(), firstSlice, numSlices);

   // Create input/output buffers
   auto untiledSize = static_cast<uint32_t>(untiled.size());
   auto uploadBuffer = allocateSsboBuffer(surfSize, SsboBufferUsage::CpuToGpu);
   auto inputBuffer = allocateSsboBuffer(untiledSize, SsboBufferUsage::Gpu);
   auto outputBuffer = allocateSsboBuffer(untiledSize, SsboBufferUsage::Gpu);
   auto downloadBuffer = allocateSsboBuffer(surfSize, SsboBufferUsage::CpuToGpu);

   // Upload to the upload buffer
   uploadSsboBuffer(uploadBuffer, untiled.data(), surfSize);

   {
      // Allocate a command buffer and fence
      auto cmdBuffer = allocSyncCmdBuffer();
      beginSyncCmdBuffer(cmdBuffer);

      // Barrier our host writes the transfer reads
      globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead);

      // Clear the input/output buffers to a known value so its obvious if something goes wrong
      cmdBuffer.cmds.fillBuffer(inputBuffer.buffer, 0, surfSize, 0xffffffff);
      cmdBuffer.cmds.fillBuffer(inputBuffer.buffer, surfSize, untiledSize - surfSize, 0xfefefefe);
      cmdBuffer.cmds.fillBuffer(outputBuffer.buffer, 0, surfSize, 0x00000000);
      cmdBuffer.cmds.fillBuffer(outputBuffer.buffer, surfSize, untiledSize - surfSize, 0x01010101);

      // Set up the input/output buffers on the GPU
      cmdBuffer.cmds.copyBuffer(uploadBuffer.buffer, inputBuffer.buffer, { vk::BufferCopy(0, 0, surfSize) });

      // Barrier our transfers to the shader reads
      globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);

      // Dispatch the actual retile
      auto retileInfo = gpu7::tiling::vulkan::calculateRetileInfo(desc, firstSlice, numSlices);

      auto tiledFirstSlice = align_down(firstSlice, retileInfo.microTileThickness);
      auto tiledOffset = tiledFirstSlice * retileInfo.sliceBytes / retileInfo.microTileThickness;
      auto untiledOffset = firstSlice * retileInfo.sliceBytes / retileInfo.microTileThickness;

      auto handle = gVkRetiler.untile(cmdBuffer.cmds,
                                      outputBuffer.buffer, untiledOffset,
                                      inputBuffer.buffer, tiledOffset,
                                      retileInfo);

      // Put a barrier from the shader writes to the transfers
      globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead);

      // Copy the output buffer to the download buffer
      cmdBuffer.cmds.copyBuffer(outputBuffer.buffer, downloadBuffer.buffer, { vk::BufferCopy(0, 0, surfSize) });

      // Put a barrier from the transfer writes to the host reads
      globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eHostRead);

      // End, execute and free this buffer
      endSyncCmdBuffer(cmdBuffer);
      execSyncCmdBuffer(cmdBuffer);
      freeSyncCmdBuffer(cmdBuffer);

      // Free the retiler resources we used
      gVkRetiler.releaseHandle(handle);
   }

   // Capture the retiled data from the GPU
   std::vector<uint8_t> outputData;
   outputData.resize(surfSize);
   downloadSsboBuffer(downloadBuffer, outputData.data(), surfSize);

   // Compare that the images match
   CHECK(compareImages(outputData, addrLibImage));

   // Free the buffers associated with this
   freeSsboBuffer(uploadBuffer);
   freeSsboBuffer(inputBuffer);
   freeSsboBuffer(outputBuffer);
   freeSsboBuffer(downloadBuffer);
}

TEST_CASE("vkTiling", "")
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

struct PendingVkPerfEntry
{
   gpu7::tiling::SurfaceDescription desc;
   uint32_t firstSlice;
   uint32_t numSlices;

   ADDR_COMPUTE_SURFACE_INFO_OUTPUT info;
   SsboBuffer uploadBuffer;
   SsboBuffer inputBuffer;
   SsboBuffer outputBuffer;

   gpu7::tiling::vulkan::RetileHandle handle;
};

TEST_CASE("vkTilingPerf", "[!benchmark]")
{
   // Set up AddrLib to generate data to test against
   auto addrLib = AddrLib { };

   // Get some random data to use
   auto &untiled = sRandomData;

   // Some place to store pending tests
   std::vector<PendingVkPerfEntry> pendingTests;

   // Generate all the test cases to run
   auto &layout = sPerfTestLayout;
   for (auto &mode : sTestTilingMode) {
      for (auto &format : sTestFormats) {
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

         PendingVkPerfEntry test;
         test.desc = surface;
         test.firstSlice = layout.testFirstSlice;
         test.numSlices = layout.testNumSlices;
         pendingTests.push_back(test);
      }
   }

   // Set up all the tests
   for (auto &test : pendingTests) {
      // Compute some needed surface information
      auto info = addrLib.computeSurfaceInfo(test.desc, 0, 0);
      auto surfSize = static_cast<uint32_t>(info.surfSize);

      // Make sure our test data is big enough
      REQUIRE(untiled.size() >= info.surfSize);

      // Create input/output buffers
      auto untiledSize = static_cast<uint32_t>(untiled.size());
      auto uploadBuffer = allocateSsboBuffer(surfSize, SsboBufferUsage::CpuToGpu);
      auto inputBuffer = allocateSsboBuffer(surfSize, SsboBufferUsage::Gpu);
      auto outputBuffer = allocateSsboBuffer(surfSize, SsboBufferUsage::Gpu);

      // Upload to the upload buffer
      uploadSsboBuffer(uploadBuffer, untiled.data(), surfSize);

      // Store the state between setup loops
      test.info = info;
      test.uploadBuffer = uploadBuffer;
      test.inputBuffer = inputBuffer;
      test.outputBuffer = outputBuffer;
   }

   // Copy our uploaded data to the input buffers
   {
      auto cmdBuffer = allocSyncCmdBuffer();
      beginSyncCmdBuffer(cmdBuffer);

      for (auto &test : pendingTests) {
         auto surfSize = static_cast<uint32_t>(test.info.surfSize);

         // Barrier our host writes the transfer reads
         globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eHostWrite, vk::AccessFlagBits::eTransferRead);

         // Set up the input/output buffers on the GPU
         cmdBuffer.cmds.copyBuffer(test.uploadBuffer.buffer,
                                   test.inputBuffer.buffer,
                                   { vk::BufferCopy(0, 0, surfSize) });

         // Barrier our transfers to the shader reads
         globalVkMemoryBarrier(cmdBuffer.cmds, vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eShaderRead);
      }

      endSyncCmdBuffer(cmdBuffer);
      execSyncCmdBuffer(cmdBuffer);
      freeSyncCmdBuffer(cmdBuffer);
   }

   // Run the retiles
   {
      auto cmdBuffer = allocSyncCmdBuffer();
      beginSyncCmdBuffer(cmdBuffer);

      for (auto &test : pendingTests) {
         auto surfSize = static_cast<uint32_t>(test.info.surfSize);

         // Calculate data on how to retile
         auto retileInfo = gpu7::tiling::vulkan::calculateRetileInfo(test.desc, test.firstSlice, test.numSlices);

         auto tiledFirstSlice = align_down(test.firstSlice, retileInfo.microTileThickness);
         auto tiledOffset = tiledFirstSlice * retileInfo.sliceBytes / retileInfo.microTileThickness;
         auto untiledOffset = test.firstSlice * retileInfo.sliceBytes / retileInfo.microTileThickness;

         // Dispatch the actual retile
         auto handle = gVkRetiler.untile(cmdBuffer.cmds,
                                         test.outputBuffer.buffer, untiledOffset,
                                         test.inputBuffer.buffer, tiledOffset,
                                         retileInfo);

         // Save some information for freeing later
         test.handle = handle;
      }

      endSyncCmdBuffer(cmdBuffer);

      auto benchTitle = fmt::format("processing ({} retiles)", pendingTests.size());
      BENCHMARK(benchTitle)
      {
         execSyncCmdBuffer(cmdBuffer);
      }

      freeSyncCmdBuffer(cmdBuffer);
   }

   // Clean up all our resources used...
   for (auto &test : pendingTests) {
      // Free the retiler resources we used
      gVkRetiler.releaseHandle(test.handle);

      // Free the buffers we allocated
      freeSsboBuffer(test.uploadBuffer);
      freeSsboBuffer(test.inputBuffer);
      freeSsboBuffer(test.outputBuffer);
   }
}

bool vulkanBeforeStart()
{
   if (!initialiseVulkan()) {
      return false;
   }

   // Initialize our retiler
   gVkRetiler.initialise(gDevice);
   return true;
}

bool vulkanAfterComplete()
{
   return shutdownVulkan();
}

#endif // DECAF_VULKAN
