#pragma once

#include <addrlib/addrinterface.h>
#include <catch.hpp>
#include <cstring>
#include <libgpu/gpu7_tiling.h>

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
      untileSlices(const gpu7::tiling::SurfaceDescription &desc,
                   uint32_t mipLevel,
                   const uint8_t *src,
                   uint8_t *dst,
                   uint32_t firstSlice,
                   uint32_t numSlices)
   {
      for (uint32_t sample = 0; sample < desc.numSamples; ++sample) {
         for (uint32_t slice = firstSlice; slice < firstSlice + numSlices; ++slice) {
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
      tileSlices(const gpu7::tiling::SurfaceDescription &desc,
                 uint32_t mipLevel,
                 uint8_t *src,
                 uint8_t *dst,
                 uint32_t firstSlice,
                 uint32_t numSlices)
   {
      for (uint32_t sample = 0; sample < desc.numSamples; ++sample) {
         for (uint32_t slice = firstSlice; slice < firstSlice + numSlices; ++slice) {
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
