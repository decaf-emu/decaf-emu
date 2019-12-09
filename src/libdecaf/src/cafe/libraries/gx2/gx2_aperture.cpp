#include "gx2.h"
#include "gx2_aperture.h"
#include "gx2_format.h"
#include "gx2_surface.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"
#include "cafe/libraries/tcl/tcl_aperture.h"

#include <libgpu/gpu7_tiling.h>
#include <libgpu/gpu7_tiling_cpu.h>

namespace cafe::gx2
{

using namespace cafe::coreinit;
using namespace cafe::tcl;

struct ApertureInfo
{
   gpu7::tiling::RetileInfo retileInfo;
   be2_val<virt_addr> tiledAddress;
   be2_val<uint32_t> tiledSize;
   be2_val<virt_addr> untiledAddress;
   be2_val<uint32_t> untiledSize;
   be2_val<uint32_t> slice;
   be2_val<uint32_t> numSlices;
};

struct StaticApertureData
{
   be2_array<ApertureInfo, 32> apertureInfo;
};

static virt_ptr<StaticApertureData>
sApertureData = nullptr;

void
GX2AllocateTilingApertureEx(virt_ptr<GX2Surface> surface,
                            uint32_t level,
                            uint32_t depth,
                            GX2EndianSwapMode endian,
                            virt_ptr<GX2ApertureHandle> outHandle,
                            virt_ptr<virt_addr> outAddress)
{
   auto surfaceDescription = gpu7::tiling::SurfaceDescription{ };
   surfaceDescription.tileMode = static_cast<gpu7::tiling::TileMode>(surface->tileMode);
   surfaceDescription.format = static_cast<gpu7::tiling::DataFormat>(surface->format & 0x3f);
   surfaceDescription.bpp = GX2GetSurfaceFormatBitsPerElement(surface->format) / 8;
   surfaceDescription.numSlices = surface->depth;
   surfaceDescription.numSamples = 1 << surface->aa;
   surfaceDescription.numFrags = 1 << surface->aa;
   surfaceDescription.numLevels = surface->mipLevels;
   surfaceDescription.pipeSwizzle = (surface->swizzle >> 8) & 1;
   surfaceDescription.bankSwizzle = (surface->swizzle >> 9) & 3;
   surfaceDescription.width = surface->width;
   surfaceDescription.height = surface->height;
   surfaceDescription.use = static_cast<gpu7::tiling::SurfaceUse>(surface->use);
   surfaceDescription.dim = static_cast<gpu7::tiling::SurfaceDim>(surface->dim);

   if (GX2SurfaceIsCompressed(surface->format)) {
      surfaceDescription.width = (surfaceDescription.width + 3) / 4;
      surfaceDescription.height = (surfaceDescription.height + 3) / 4;
   }

   auto surfaceInfo = gpu7::tiling::computeSurfaceInfo(surfaceDescription, level);

   auto addr = virt_addr { 0 };
   if (level == 0) {
      addr = virt_cast<virt_addr>(surface->image);
   } else if (level == 1) {
      addr = virt_cast<virt_addr>(surface->mipmaps);
   } else if (level > 1) {
      addr = virt_cast<virt_addr>(surface->mipmaps) + surface->mipLevelOffset[level - 1];
   }

   // TODO: (addr + surfaceInfo.sliceSize * depth) is not actually correct for
   //       all tile modes.

   if (TCLAllocTilingAperture(OSEffectiveToPhysical(addr + surfaceInfo.sliceSize * depth),
                              surfaceInfo.pitch,
                              surfaceInfo.height,
                              surfaceInfo.bpp,
                              surfaceInfo.tileMode,
                              endian,
                              outHandle,
                              outAddress) == TCLStatus::OK) {
      auto &info = sApertureData->apertureInfo[*outHandle];
      info.retileInfo = gpu7::tiling::computeRetileInfo(surfaceInfo);
      info.tiledAddress = addr;
      info.tiledSize = surfaceInfo.sliceSize;
      info.untiledAddress = *outAddress;
      info.untiledSize = surfaceInfo.sliceSize;
      info.slice = depth;
      info.numSlices = 1u;

      gpu7::tiling::cpu::untile(info.retileInfo,
                                virt_cast<uint8_t *>(info.untiledAddress).get(),
                                virt_cast<uint8_t *>(info.tiledAddress).get(),
                                info.slice,
                                info.numSlices);
   }
}

void
GX2FreeTilingAperture(GX2ApertureHandle handle)
{
   auto &info = sApertureData->apertureInfo[handle];
   if (info.tiledAddress && info.untiledAddress) {
      gpu7::tiling::cpu::tile(info.retileInfo,
                              virt_cast<uint8_t *>(info.untiledAddress).get(),
                              virt_cast<uint8_t *>(info.tiledAddress).get(),
                              info.slice,
                              info.numSlices);
   }

   TCLFreeTilingAperture(handle);
   info.tiledAddress = virt_addr { 0u };
   info.tiledSize = 0u;
   info.untiledAddress = virt_addr { 0u };
   info.untiledSize = 0u;
}

namespace internal
{

bool
translateAperture(virt_addr &address,
                  uint32_t &size)
{
   for (auto i = 0u; i < sApertureData->apertureInfo.size(); ++i) {
      auto &info = sApertureData->apertureInfo[i];
      if (address >= info.untiledAddress &&
          address < info.untiledAddress + info.untiledSize) {
         address = info.tiledAddress;
         size = info.tiledSize;
         return true;
      }
   }

   return false;
}

} // namespace internal

void
Library::registerApertureSymbols()
{
   RegisterFunctionExport(GX2AllocateTilingApertureEx);
   RegisterFunctionExport(GX2FreeTilingAperture);

   RegisterDataInternal(sApertureData);
}

} // namespace cafe::gx2
