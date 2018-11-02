#include "gpu_addrlibopt.h"
#include "gpu_tiling.h"

#include <common/decaf_assert.h>
#include <cstdlib>
#include <cstring>

namespace gpu
{

static const bool
USE_ADDRLIBOPT = true;

static ADDR_HANDLE
gAddrLibHandle = nullptr;

static void *
allocSysMem(const ADDR_ALLOCSYSMEM_INPUT *pInput)
{
   return std::malloc(pInput->sizeInBytes);
}

static ADDR_E_RETURNCODE
freeSysMem(const ADDR_FREESYSMEM_INPUT *pInput)
{
   std::free(pInput->pVirtAddr);
   return ADDR_OK;
}

bool
initAddrLib()
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

   input.callbacks.allocSysMem = &allocSysMem;
   input.callbacks.freeSysMem = &freeSysMem;

   auto result = AddrCreate(&input, &output);

   if (result != ADDR_OK) {
      return false;
   }

   gAddrLibHandle = output.hLib;
   return true;
}

ADDR_HANDLE
getAddrLibHandle()
{
   if (!gAddrLibHandle) {
      initAddrLib();
   }

   return gAddrLibHandle;
}

static inline void
calcSurfaceBankPipeSwizzle(uint32_t swizzle, uint32_t *bankSwizzle, uint32_t *pipeSwizzle)
{
   auto handle = getAddrLibHandle();

   ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT input;
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT output;

   std::memset(&input, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT));
   std::memset(&output, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT));

   input.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT);
   output.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT);

   input.base256b = (swizzle >> 8) & 0xFF;
   AddrExtractBankPipeSwizzle(handle, &input, &output);

   *bankSwizzle = output.bankSwizzle;
   *pipeSwizzle = output.pipeSwizzle;
}

void
alignTiling(latte::SQ_TILE_MODE& tileMode,
            latte::SQ_DATA_FORMAT& format,
            uint32_t& swizzle,
            uint32_t& pitch,
            uint32_t& width,
            uint32_t& height,
            uint32_t& depth,
            uint32_t& aa,
            bool& isDepth,
            uint32_t& bpp)
{
   auto handle = getAddrLibHandle();

   // We only partially complete this, knowing that we only need
   // some information which does not depend on it...
   ADDR_COMPUTE_SURFACE_INFO_INPUT input;
   memset(&input, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT));
   input.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT);
   input.tileMode = static_cast<AddrTileMode>(tileMode);
   input.format = static_cast<AddrFormat>(format);
   input.bpp = bpp;
   input.width = pitch;
   input.height = height;
   input.numSlices = depth;
   input.numSamples = 1;
   input.numFrags = 0;
   input.slice = 0;
   input.mipLevel = 0;

   ADDR_COMPUTE_SURFACE_INFO_OUTPUT output;
   std::memset(&output, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT));
   output.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT);

   auto result = AddrComputeSurfaceInfo(gpu::getAddrLibHandle(), &input, &output);
   decaf_check(result == ADDR_OK);

   pitch = output.pitch;
   height = output.height;
}

bool
copySurfacePixels(uint8_t *dstBasePtr,
   uint32_t dstWidth,
   uint32_t dstHeight,
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &dstAddrInput,
   uint8_t *srcBasePtr,
   uint32_t srcWidth,
   uint32_t srcHeight,
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT &srcAddrInput)
{
   auto handle = getAddrLibHandle();

   decaf_check(srcAddrInput.bpp == dstAddrInput.bpp);
   auto bpp = dstAddrInput.bpp;

   if (USE_ADDRLIBOPT) {
      decaf_check(srcAddrInput.isDepth == dstAddrInput.isDepth);
      decaf_check(srcAddrInput.numSamples == dstAddrInput.numSamples);
      auto isDepth = dstAddrInput.isDepth;
      auto numSamples = dstAddrInput.numSamples;

      return gpu::addrlibopt::copySurfacePixels(
         dstBasePtr, dstWidth, dstHeight, dstAddrInput,
         srcBasePtr, srcWidth, srcHeight, srcAddrInput,
         bpp, isDepth, numSamples);
   } else {
      ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT srcAddrOutput;
      ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT dstAddrOutput;

      std::memset(&srcAddrOutput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT));
      std::memset(&dstAddrOutput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT));

      srcAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);
      dstAddrOutput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_OUTPUT);

      for (auto y = 0u; y < dstHeight; ++y) {
         for (auto x = 0u; x < dstWidth; ++x) {
            srcAddrInput.x = srcWidth * x / dstWidth;
            srcAddrInput.y = srcHeight * y / dstHeight;
            AddrComputeSurfaceAddrFromCoord(handle, &srcAddrInput, &srcAddrOutput);

            dstAddrInput.x = x;
            dstAddrInput.y = y;
            AddrComputeSurfaceAddrFromCoord(handle, &dstAddrInput, &dstAddrOutput);

            auto src = &srcBasePtr[srcAddrOutput.addr];
            auto dst = &dstBasePtr[dstAddrOutput.addr];
            std::memcpy(dst, src, bpp / 8);
         }
      }

      return true;
   }
}

bool
convertFromTiled(
   uint8_t *output,
   uint32_t outputPitch,
   uint8_t *input,
   latte::SQ_TILE_MODE tileMode,
   uint32_t swizzle,
   uint32_t pitch,
   uint32_t width,
   uint32_t height,
   uint32_t depth,
   uint32_t aa,
   bool isDepth,
   uint32_t bpp,
   uint32_t beginSlice,
   uint32_t endSlice)
{
   if (endSlice == 0) {
      endSlice = depth;
   }

   decaf_check(beginSlice >= 0);
   decaf_check(endSlice > 0);
   decaf_check(endSlice <= depth);

   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT srcAddrInput;
   std::memset(&srcAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));
   srcAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
   srcAddrInput.bpp = bpp;
   srcAddrInput.pitch = pitch;
   srcAddrInput.height = height;
   srcAddrInput.numSlices = depth;
   srcAddrInput.numSamples = 1 << aa;
   srcAddrInput.tileMode = static_cast<AddrTileMode>(tileMode);
   srcAddrInput.isDepth = isDepth;
   srcAddrInput.tileBase = 0;
   srcAddrInput.compBits = 0;
   srcAddrInput.numFrags = 0;
   calcSurfaceBankPipeSwizzle(swizzle,
      &srcAddrInput.bankSwizzle,
      &srcAddrInput.pipeSwizzle);

   // Setup dst
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT dstAddrInput;
   std::memset(&dstAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));
   dstAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
   dstAddrInput.bpp = bpp;
   dstAddrInput.pitch = outputPitch;
   dstAddrInput.height = height;
   dstAddrInput.numSlices = depth;
   dstAddrInput.numSamples = 1;
   dstAddrInput.tileMode = AddrTileMode::ADDR_TM_LINEAR_GENERAL;
   dstAddrInput.isDepth = isDepth;
   dstAddrInput.tileBase = 0;
   dstAddrInput.compBits = 0;
   dstAddrInput.numFrags = 0;
   dstAddrInput.bankSwizzle = 0;
   dstAddrInput.pipeSwizzle = 0;

   // Untiling always takes sample 0
   srcAddrInput.sample = 0;
   dstAddrInput.sample = 0;

   // Untile all of the slices of this surface
   for (uint32_t slice = beginSlice; slice < endSlice; ++slice) {
      srcAddrInput.slice = slice;
      dstAddrInput.slice = slice;

      copySurfacePixels(
         output, width, height, dstAddrInput,
         input, width, height, srcAddrInput);
   }

   return true;
}

bool
convertToTiled(
   uint8_t *output,
   uint8_t *input,
   uint32_t inputPitch,
   latte::SQ_TILE_MODE tileMode,
   uint32_t swizzle,
   uint32_t pitch,
   uint32_t width,
   uint32_t height,
   uint32_t depth,
   uint32_t aa,
   bool isDepth,
   uint32_t bpp,
   uint32_t beginSlice,
   uint32_t endSlice)
{
   if (endSlice == 0) {
      endSlice = depth;
   }

   decaf_check(beginSlice >= 0);
   decaf_check(endSlice > 0);
   decaf_check(endSlice <= depth);

   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT srcAddrInput;
   std::memset(&srcAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));
   srcAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
   srcAddrInput.bpp = bpp;
   srcAddrInput.pitch = inputPitch;
   srcAddrInput.height = height;
   srcAddrInput.numSlices = depth;
   srcAddrInput.numSamples = 1;
   srcAddrInput.tileMode = AddrTileMode::ADDR_TM_LINEAR_GENERAL;
   srcAddrInput.isDepth = isDepth;
   srcAddrInput.tileBase = 0;
   srcAddrInput.compBits = 0;
   srcAddrInput.numFrags = 0;
   srcAddrInput.bankSwizzle = 0;
   srcAddrInput.pipeSwizzle = 0;

   // Setup dst
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT dstAddrInput;
   std::memset(&dstAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));
   dstAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
   dstAddrInput.bpp = bpp;
   dstAddrInput.pitch = pitch;
   dstAddrInput.height = height;
   dstAddrInput.numSlices = depth;
   dstAddrInput.numSamples = 1 << aa;
   dstAddrInput.tileMode = static_cast<AddrTileMode>(tileMode);
   dstAddrInput.isDepth = isDepth;
   dstAddrInput.tileBase = 0;
   dstAddrInput.compBits = 0;
   dstAddrInput.numFrags = 0;
   calcSurfaceBankPipeSwizzle(swizzle,
                              &srcAddrInput.bankSwizzle,
                              &srcAddrInput.pipeSwizzle);

   // Untiling always takes sample 0
   srcAddrInput.sample = 0;
   dstAddrInput.sample = 0;

   // Untile all of the slices of this surface
   for (uint32_t slice = beginSlice; slice < endSlice; ++slice) {
      srcAddrInput.slice = slice;
      dstAddrInput.slice = slice;

      copySurfacePixels(
         output, width, height, dstAddrInput,
         input, width, height, srcAddrInput);
   }

   return true;
}

} // namespace gpu
