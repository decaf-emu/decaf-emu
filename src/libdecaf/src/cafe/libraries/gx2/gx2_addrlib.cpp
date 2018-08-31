#include "gx2_addrlib.h"
#include "gx2_surface.h"
#include "gx2_format.h"

#include <cstdlib>
#include <cstring>
#include <common/align.h>
#include <libgpu/gpu_tiling.h>

namespace cafe::gx2::internal
{

bool
getSurfaceInfo(GX2Surface *surface,
               uint32_t level,
               ADDR_COMPUTE_SURFACE_INFO_OUTPUT *output)
{
   ADDR_E_RETURNCODE result = ADDR_OK;
   auto hwFormat = static_cast<latte::SQ_DATA_FORMAT>(surface->format & 0x3f);
   auto height = 1u;
   auto width = std::max<uint32_t>(1u, surface->width >> level);
   auto numSlices = 1u;

   switch (surface->dim) {
   case GX2SurfaceDim::Texture1D:
      height = 1;
      numSlices = 1;
      break;
   case GX2SurfaceDim::Texture2D:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = 1;
      break;
   case GX2SurfaceDim::Texture3D:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = std::max<uint32_t>(1u, surface->depth >> level);
      break;
   case GX2SurfaceDim::TextureCube:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = std::max<uint32_t>(6u, surface->depth);
      break;
   case GX2SurfaceDim::Texture1DArray:
      height = 1;
      numSlices = surface->depth;
      break;
   case GX2SurfaceDim::Texture2DArray:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = surface->depth;
      break;
   case GX2SurfaceDim::Texture2DMSAA:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = 1;
      break;
   case GX2SurfaceDim::Texture2DMSAAArray:
      height = std::max<uint32_t>(1u, surface->height >> level);
      numSlices = surface->depth;
      break;
   }

   std::memset(output, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT));
   output->size = sizeof(ADDR_COMPUTE_SURFACE_INFO_OUTPUT);

   if (surface->tileMode == GX2TileMode::LinearSpecial) {
      auto numSamples = 1 << surface->aa;
      auto elemSize = 1u;

      if (hwFormat >= latte::SQ_DATA_FORMAT::FMT_BC1 && hwFormat <= latte::SQ_DATA_FORMAT::FMT_BC5) {
         elemSize = 4;
      }

      output->bpp = GX2GetSurfaceFormatBitsPerElement(surface->format);
      output->pixelBits = output->bpp;
      output->baseAlign = 1;
      output->pitchAlign = 1;
      output->heightAlign = 1;
      output->depthAlign = 1;

      width = align_up(width, elemSize);
      height = align_up(height, elemSize);

      output->depth = numSlices;
      output->pitch = std::max<uint32_t>(1u, width / elemSize);
      output->height = std::max<uint32_t>(1u, height / elemSize);

      output->pixelPitch = width;
      output->pixelHeight = height;

      output->surfSize = static_cast<uint64_t>(output->height) * output->pitch * numSamples * output->depth * (output->bpp / 8);

      if (surface->dim == GX2SurfaceDim::Texture3D) {
         output->sliceSize = static_cast<uint32_t>(output->surfSize);
      } else {
         output->sliceSize = static_cast<uint32_t>(output->surfSize / output->depth);
      }

      output->pitchTileMax = (output->pitch / 8) - 1;
      output->heightTileMax = (output->height / 8) - 1;
      output->sliceTileMax = (output->height * output->pitch / 64) - 1;
      result = ADDR_OK;
   } else {
      ADDR_COMPUTE_SURFACE_INFO_INPUT input;
      memset(&input, 0, sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT));
      input.size = sizeof(ADDR_COMPUTE_SURFACE_INFO_INPUT);
      input.tileMode = static_cast<AddrTileMode>(surface->tileMode & 0xF);
      input.format = static_cast<AddrFormat>(hwFormat);
      input.bpp = GX2GetSurfaceFormatBitsPerElement(surface->format);
      input.width = width;
      input.height = height;
      input.numSlices = numSlices;

      input.numSamples = 1 << surface->aa;
      input.numFrags = input.numSamples;

      input.slice = 0;
      input.mipLevel = level;

      if (surface->dim == GX2SurfaceDim::TextureCube) {
         input.flags.cube = 1;
      }

      if (surface->use & GX2SurfaceUse::DepthBuffer) {
         input.flags.depth = 1;
      }

      if (surface->use & GX2SurfaceUse::ScanBuffer) {
         input.flags.display = 1;
      }

      if (surface->dim == GX2SurfaceDim::Texture3D) {
         input.flags.volume = 1;
      }

      input.flags.inputBaseMap = (level == 0);
      result = AddrComputeSurfaceInfo(gpu::getAddrLibHandle(), &input, output);
   }

   return (result == ADDR_OK);
}

bool
copySurface(GX2Surface *surfaceSrc,
            uint32_t srcLevel,
            uint32_t srcSlice,
            GX2Surface *surfaceDst,
            uint32_t dstLevel,
            uint32_t dstSlice,
            uint8_t *dstImage,
            uint8_t *dstMipmap)
{
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT srcInfoOutput;
   ADDR_COMPUTE_SURFACE_INFO_OUTPUT dstInfoOutput;
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT srcAddrInput;
   ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT dstAddrInput;
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT srcSwizzleInput;
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT srcSwizzleOutput;
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT dstSwizzleInput;
   ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT dstSwizzleOutput;

   auto handle = gpu::getAddrLibHandle();

   // Initialise addrlib input/output structures
   std::memset(&srcAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));
   std::memset(&dstAddrInput, 0, sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT));

   std::memset(&srcSwizzleInput, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT));
   std::memset(&srcSwizzleOutput, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT));

   std::memset(&dstSwizzleInput, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT));
   std::memset(&dstSwizzleOutput, 0, sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT));

   srcAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);
   dstAddrInput.size = sizeof(ADDR_COMPUTE_SURFACE_ADDRFROMCOORD_INPUT);

   srcSwizzleInput.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT);
   srcSwizzleOutput.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT);

   dstSwizzleInput.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_INPUT);
   dstSwizzleOutput.size = sizeof(ADDR_EXTRACT_BANKPIPE_SWIZZLE_OUTPUT);

   // Setup src
   auto bpp = GX2GetSurfaceFormatBitsPerElement(surfaceSrc->format);
   getSurfaceInfo(surfaceSrc, srcLevel, &srcInfoOutput);
   srcAddrInput.slice = srcSlice;
   srcAddrInput.sample = 0;
   srcAddrInput.bpp = bpp;
   srcAddrInput.pitch = srcInfoOutput.pitch;
   srcAddrInput.height = srcInfoOutput.height;
   srcAddrInput.numSlices = std::max<uint32_t>(1u, srcInfoOutput.depth);
   srcAddrInput.numSamples = 1 << surfaceSrc->aa;
   srcAddrInput.tileMode = static_cast<AddrTileMode>(surfaceSrc->tileMode.value());
   srcAddrInput.isDepth = !!(surfaceSrc->use & GX2SurfaceUse::DepthBuffer);
   srcAddrInput.tileBase = 0;
   srcAddrInput.compBits = 0;
   srcAddrInput.numFrags = 0;

   // Setup src swizzle
   srcSwizzleInput.base256b = (surfaceSrc->swizzle >> 8) & 0xFF;
   AddrExtractBankPipeSwizzle(handle, &srcSwizzleInput, &srcSwizzleOutput);

   srcAddrInput.bankSwizzle = srcSwizzleOutput.bankSwizzle;
   srcAddrInput.pipeSwizzle = srcSwizzleOutput.pipeSwizzle;

   // Setup dst
   getSurfaceInfo(surfaceDst, srcLevel, &dstInfoOutput);
   dstAddrInput.slice = srcSlice;
   dstAddrInput.sample = 0;
   dstAddrInput.bpp = bpp;
   dstAddrInput.pitch = dstInfoOutput.pitch;
   dstAddrInput.height = dstInfoOutput.height;
   dstAddrInput.numSlices = std::max<uint32_t>(1u, dstInfoOutput.depth);
   dstAddrInput.numSamples = 1 << surfaceDst->aa;
   dstAddrInput.tileMode = dstInfoOutput.tileMode;
   dstAddrInput.isDepth = !!(surfaceDst->use & GX2SurfaceUse::DepthBuffer);
   dstAddrInput.tileBase = 0;
   dstAddrInput.compBits = 0;
   dstAddrInput.numFrags = 0;

   // Setup dst swizzle
   dstSwizzleInput.base256b = (surfaceDst->swizzle >> 8) & 0xFF;
   AddrExtractBankPipeSwizzle(handle, &dstSwizzleInput, &dstSwizzleOutput);

   dstAddrInput.bankSwizzle = dstSwizzleOutput.bankSwizzle;
   dstAddrInput.pipeSwizzle = dstSwizzleOutput.pipeSwizzle;

   // Setup width
   auto srcWidth = std::max<uint32_t>(1u, surfaceSrc->width >> srcLevel);
   auto srcHeight = std::max<uint32_t>(1u, surfaceSrc->height >> srcLevel);
   auto hwFormatSrc = static_cast<latte::SQ_DATA_FORMAT>(surfaceSrc->format & 0x3F);

   if (hwFormatSrc >= latte::SQ_DATA_FORMAT::FMT_BC1 && hwFormatSrc <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      srcWidth = (srcWidth + 3) / 4;
      srcHeight = (srcHeight + 3) / 4;
   }

   auto dstWidth = std::max<uint32_t>(1u, surfaceDst->width >> dstLevel);
   auto dstHeight = std::max<uint32_t>(1u, surfaceDst->height >> dstLevel);
   auto hwFormatDst = static_cast<latte::SQ_DATA_FORMAT>(surfaceDst->format & 0x3F);

   if (hwFormatDst >= latte::SQ_DATA_FORMAT::FMT_BC1 && hwFormatDst <= latte::SQ_DATA_FORMAT::FMT_BC5) {
      dstWidth = (dstWidth + 3) / 4;
      dstHeight = (dstHeight + 3) / 4;
   }

   uint8_t *srcBasePtr = nullptr;
   uint8_t *dstBasePtr = nullptr;

   if (srcLevel == 0) {
      srcBasePtr = surfaceSrc->image.getRawPointer();
   } else if (srcLevel == 1) {
      srcBasePtr = surfaceSrc->mipmaps.getRawPointer();
   } else {
      srcBasePtr = surfaceSrc->mipmaps.getRawPointer() + surfaceSrc->mipLevelOffset[srcLevel - 1];
   }

   if (dstLevel == 0) {
      dstBasePtr = dstImage ? dstImage : surfaceDst->image.getRawPointer();
   } else if (dstLevel == 1) {
      dstBasePtr = dstMipmap ? dstMipmap : surfaceDst->mipmaps.getRawPointer();
   } else {
      dstBasePtr = dstMipmap ? dstMipmap : surfaceDst->mipmaps.getRawPointer();
      dstBasePtr += surfaceDst->mipLevelOffset[dstLevel - 1];
   }

   // LinearSpecial is a special mode available through GX2 which forces that the
   //  tiling mode be linear (possibly unaligned).  Because this also signals the
   //  surface info calculation to pick its own tiling, we need to handle this
   //  specially here...  Note that while ADDR_TM_LINEAR_GENERAL is a valid AMD
   //  specified mode, it is not actually normally supported by the R600 hardware
   //  which is the reason for LinearSpecial (which forces SW handling in GX2).

   if (surfaceSrc->tileMode == GX2TileMode::LinearSpecial) {
      srcAddrInput.tileMode = AddrTileMode::ADDR_TM_LINEAR_GENERAL;
   }

   if (surfaceDst->tileMode == GX2TileMode::LinearSpecial) {
      dstAddrInput.tileMode = AddrTileMode::ADDR_TM_LINEAR_GENERAL;
   }

   return gpu::copySurfacePixels(
      dstBasePtr, dstWidth, dstHeight, dstAddrInput,
      srcBasePtr, srcWidth, srcHeight, srcAddrInput);
}

uint32_t
getSurfaceSliceSwizzle(GX2TileMode tileMode,
                       uint32_t baseSwizzle,
                       uint32_t slice)
{
   ADDR_COMPUTE_SLICESWIZZLE_INPUT input;
   ADDR_COMPUTE_SLICESWIZZLE_OUTPUT output;
   auto tileSwizzle = uint32_t { 0 };

   std::memset(&input, 0, sizeof(ADDR_COMPUTE_SLICESWIZZLE_INPUT));
   std::memset(&output, 0, sizeof(ADDR_COMPUTE_SLICESWIZZLE_OUTPUT));

   input.size = sizeof(ADDR_COMPUTE_SLICESWIZZLE_INPUT);
   output.size = sizeof(ADDR_COMPUTE_SLICESWIZZLE_OUTPUT);

   if (tileMode >= GX2TileMode::Tiled2DThin1 && tileMode != GX2TileMode::LinearSpecial) {
      input.tileMode = static_cast<AddrTileMode>(tileMode);
      input.baseSwizzle = baseSwizzle;
      input.slice = slice;
      input.baseAddr = 0;

      auto handle = gpu::getAddrLibHandle();
      AddrComputeSliceSwizzle(handle, &input, &output);
      tileSwizzle = output.tileSwizzle;
   }

   return tileSwizzle;
}

uint32_t
calcSliceSize(GX2Surface *surface,
              ADDR_COMPUTE_SURFACE_INFO_OUTPUT *info)
{
   return info->pitch * info->height * (1 << surface->aa) * (info->bpp / 8);
}

} // namespace cafe::gx2::internal
