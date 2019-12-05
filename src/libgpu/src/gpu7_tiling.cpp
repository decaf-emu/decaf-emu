#include "gpu7_tiling.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <common/align.h>
#include <common/decaf_assert.h>
#include <addrlib/addrinterface.h>

namespace gpu7::tiling
{

int
computeSurfaceBankSwappedWidth(TileMode tileMode,
                               uint32_t bpp,
                               uint32_t numSamples,
                               uint32_t pitch)
{
   const auto bytesPerSample = 8 * bpp;
   const auto samplesPerTile = SampleSplitSize / bytesPerSample;
   auto bankSwapWidth = uint32_t { 0 };
   auto slicesPerTile = uint32_t { 1 };

   if (samplesPerTile) {
      slicesPerTile = std::max<uint32_t>(1u, numSamples / samplesPerTile);
   }

   if (getMicroTileThickness(tileMode) > 1) {
      numSamples = 4;
   }

   if (tileMode == ADDR_TM_2B_TILED_THIN1 ||
       tileMode == ADDR_TM_2B_TILED_THIN2 ||
       tileMode == ADDR_TM_2B_TILED_THIN4 ||
       tileMode == ADDR_TM_2B_TILED_THICK ||
       tileMode == ADDR_TM_3B_TILED_THIN1 ||
       tileMode == ADDR_TM_3B_TILED_THICK) {
      const auto swapTiles = std::max<uint32_t>(1u, (SwapSize >> 1) / bpp);
      const auto swapWidth = swapTiles * 8 * NumBanks;

      const auto macroTileHeight = getMacroTileHeight(tileMode);
      const auto heightBytes = numSamples * macroTileHeight * bpp / slicesPerTile;
      const auto swapMax = NumPipes * NumBanks * RowSize / heightBytes;

      const auto bytesPerTileSlice = numSamples * bytesPerSample / slicesPerTile;
      const auto swapMin = PipeInterleaveBytes * 8 * NumBanks / bytesPerTileSlice;

      bankSwapWidth = std::min(swapMax, std::max(swapMin, swapWidth));

      while (bankSwapWidth >= 2 * pitch) {
         bankSwapWidth >>= 1;
      }
   }

   return bankSwapWidth;
}

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

static ADDR_HANDLE
getAddrLibHandle()
{
   static ADDR_HANDLE handle = nullptr;
   if (!handle) {
      auto input = ADDR_CREATE_INPUT { };
      input.size = sizeof(ADDR_CREATE_INPUT);
      input.chipEngine = CIASICIDGFXENGINE_R600;
      input.chipFamily = 0x51;
      input.chipRevision = 71;
      input.createFlags.fillSizeFields = 1;
      input.regValue.gbAddrConfig = 0x44902;
      input.callbacks.allocSysMem = &addrLibAlloc;
      input.callbacks.freeSysMem = &addrLibFree;

      auto output = ADDR_CREATE_OUTPUT { };
      output.size = sizeof(ADDR_CREATE_OUTPUT);

      if (AddrCreate(&input, &output) == ADDR_OK) {
         handle = output.hLib;
      }
   }

   return handle;
}

SurfaceInfo
computeSurfaceInfo(const SurfaceDescription &surface,
                   int mipLevel,
                   int slice)
{
   auto output = ADDR_COMPUTE_SURFACE_INFO_OUTPUT { };
   output.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT);

   auto input = ADDR_COMPUTE_SURFACE_INFO_INPUT { };
   input.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT);
   input.tileMode = static_cast<AddrTileMode>(surface.tileMode);
   input.format = static_cast<AddrFormat>(surface.format);
   input.bpp = surface.bpp;
   input.numSamples = surface.numSamples;
   input.numFrags = surface.numFrags;
   input.mipLevel = mipLevel;
   input.slice = slice;
   input.numSlices = surface.numSlices;

   input.width = std::max(surface.width >> mipLevel, 1u);
   input.height = std::max(surface.height >> mipLevel, 1u);
   input.flags.inputBaseMap = mipLevel == 0 ? 1 : 0;

   if (surface.use & SurfaceUse::ScanBuffer) {
      input.flags.display = 1;
   }

   if (surface.use & SurfaceUse::DepthBuffer) {
      input.flags.depth = 1;
   }

   if (surface.dim == SurfaceDim::Texture3D) {
      input.flags.volume = 1;
      input.numSlices = std::max(surface.numSlices >> mipLevel, 1u);
   }

   if (surface.dim == SurfaceDim::TextureCube) {
      input.flags.cube = 1;
   }

   auto handle = getAddrLibHandle();
   decaf_check(handle);
   decaf_check(AddrComputeSurfaceInfo(handle, &input, &output) == ADDR_OK);

   if (surface.dim == SurfaceDim::Texture3D) {
      output.sliceSize /= output.depth;
   }

   SurfaceInfo result;
   result.tileMode = static_cast<TileMode>(output.tileMode);
   result.bpp = output.bpp;
   result.pitch = output.pitch;
   result.height = output.height;
   result.depth = output.depth;
   result.surfSize = static_cast<uint32_t>(output.surfSize);
   result.sliceSize = output.sliceSize;
   result.baseAlign = output.baseAlign;
   result.pitchAlign = output.pitchAlign;
   result.heightAlign = output.heightAlign;
   result.depthAlign = output.depthAlign;
   return result;
}


void
unpitchImage(const SurfaceDescription &desc,
             void *pitched,
             void *unpitched)
{
   const auto info = computeSurfaceInfo(desc, 0, 0);
   const auto bytesPerElem = info.bpp / 8;
   const auto unpitchedSliceSize = desc.width * desc.height * bytesPerElem;

   for (auto slice = 0u; slice < desc.numSlices; ++slice) {
      auto src = reinterpret_cast<uint8_t *>(pitched) + info.sliceSize * slice;
      auto dst = reinterpret_cast<uint8_t *>(unpitched) + unpitchedSliceSize * slice;

      for (auto y = 0u; y < desc.height; ++y) {
         std::memcpy(dst, src, desc.width * bytesPerElem);
         src += info.pitch * bytesPerElem;
         dst += desc.width * bytesPerElem;
      }
   }
}

size_t
computeUnpitchedImageSize(const SurfaceDescription &desc)
{
   return desc.width * desc.height * (desc.bpp / 8) * desc.numSlices;
}

size_t
computeUnpitchedMipMapSize(const SurfaceDescription &desc)
{
   auto size = size_t { 0 };

   for (auto level = 1u; level < desc.numLevels; ++level) {
      const auto width = desc.width >> level;
      const auto height = desc.height >> level;
      auto numSlices = desc.numSlices;
      if (desc.dim == SurfaceDim::Texture3D) {
         numSlices >>= level;
      }

      size += width * height * (desc.bpp / 8) * numSlices;
   }

   return size;
}

void
unpitchMipMap(const SurfaceDescription &desc,
              void *pitched,
              void *unpitched)
{
   auto srcMipOffset = size_t { 0 };
   auto dstMipOffset = size_t { 0 };

   for (auto level = 1u; level < desc.numLevels; ++level) {
      const auto info = computeSurfaceInfo(desc, level, 0);
      const auto bytesPerElem = info.bpp / 8;
      const auto width = desc.width >> level;
      const auto height = desc.height >> level;
      const auto unpitchedSliceSize = width * height * bytesPerElem;
      auto numSlices = desc.numSlices;
      if (desc.dim == SurfaceDim::Texture3D) {
         numSlices >>= level;
      }

      srcMipOffset = align_up(srcMipOffset, info.baseAlign);

      for (auto slice = 0u; slice < numSlices; ++slice) {
         auto src = reinterpret_cast<uint8_t *>(pitched) + srcMipOffset + info.sliceSize * slice;
         auto dst = reinterpret_cast<uint8_t *>(unpitched) + dstMipOffset + unpitchedSliceSize * slice;

         for (auto y = 0u; y < height; ++y) {
            std::memcpy(dst, src, width * bytesPerElem);
            src += info.pitch * bytesPerElem;
            dst += width * bytesPerElem;
         }
      }

      srcMipOffset += info.surfSize;
      dstMipOffset += unpitchedSliceSize * numSlices;
   }
}

} // namespace gpu7::tiling
