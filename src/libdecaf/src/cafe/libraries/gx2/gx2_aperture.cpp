#include "gx2.h"
#include "gx2_addrlib.h"
#include "gx2_aperture.h"
#include "gx2_surface.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_memory.h"
#include "cafe/libraries/tcl/tcl_aperture.h"

namespace cafe::gx2
{

using namespace cafe::coreinit;
using namespace cafe::tcl;

struct ApertureInfo
{
   be2_struct<GX2Surface> surface;
   be2_val<virt_addr> address;
   be2_val<uint32_t> level;
   be2_val<uint32_t> depth;
   be2_val<uint32_t> size;
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
   auto surfaceInfo = StackObject<ADDR_COMPUTE_SURFACE_INFO_OUTPUT> { };

   if (endian == GX2EndianSwapMode::Default) {
      endian = GX2EndianSwapMode::None;
   }

   internal::getSurfaceInfo(surface.get(),
                            level,
                            surfaceInfo.get());

   auto addr = virt_addr { 0 };
   if (level == 0) {
      addr = virt_cast<virt_addr>(surface->image);
   } else if (level == 1) {
      addr = virt_cast<virt_addr>(surface->mipmaps);
   } else if (level > 1) {
      addr = virt_cast<virt_addr>(surface->mipmaps)
             + surface->mipLevelOffset[level - 1];
   }

   if (surface->tileMode >= GX2TileMode::Tiled2DThin1 &&
       surface->tileMode != GX2TileMode::LinearSpecial) {
      if (level < ((surface->swizzle >> 16) & 0xFF)) {
         addr ^= internal::getSurfaceSliceSwizzle(surface->tileMode,
                                                  surface->swizzle >> 8,
                                                  depth) << 8;
      } else {
         addr ^= surface->swizzle;
      }
   }

   auto sliceSize = internal::calcSliceSize(surface.get(),
                                            surfaceInfo.get());

   if (TCLAllocTilingAperture(OSEffectiveToPhysical(addr + sliceSize * depth),
                              surfaceInfo->pitch,
                              surfaceInfo->height,
                              surfaceInfo->bpp / 8,
                              surfaceInfo->tileMode,
                              endian,
                              outHandle,
                              outAddress) == TCLStatus::OK) {
      auto &info = sApertureData->apertureInfo[*outHandle];

      if (outAddress) {
         // Untile from surface memory to aperture memory
         auto apertureSurface = StackObject<GX2Surface> { };
         *apertureSurface = *surface;
         apertureSurface->mipmaps = nullptr;
         apertureSurface->image = nullptr;
         apertureSurface->tileMode = GX2TileMode::LinearSpecial;
         apertureSurface->swizzle &= 0xFFFF00FF;
         GX2CalcSurfaceSizeAndAlignment(apertureSurface);

         gx2::internal::copySurface(surface.get(),
                                    level, depth,
                                    apertureSurface.get(),
                                    level, depth,
                                    virt_cast<uint8_t *>(*outAddress).get(),
                                    virt_cast<uint8_t *>(*outAddress).get());

         info.address = *outAddress;
      } else {
         info.address = virt_addr { 0 };
      }

      info.surface = *surface;
      info.level = level;
      info.depth = depth;
      info.size = surfaceInfo->pitch * surfaceInfo->height * (surfaceInfo->bpp / 8);
   }
}

void
GX2FreeTilingAperture(GX2ApertureHandle handle)
{
   auto &info = sApertureData->apertureInfo[handle];
   if (info.address) {
      // Retile from aperture memory to surface memory
      auto apertureSurface = StackObject<GX2Surface> { };
      *apertureSurface = info.surface;
      apertureSurface->mipmaps = virt_cast<uint8_t *>(info.address);
      apertureSurface->image = virt_cast<uint8_t *>(info.address);
      apertureSurface->tileMode = GX2TileMode::LinearSpecial;
      apertureSurface->swizzle &= 0xFFFF00FF;
      GX2CalcSurfaceSizeAndAlignment(apertureSurface);

      gx2::internal::copySurface(apertureSurface.get(),
                                 info.level, info.depth,
                                 virt_addrof(info.surface).get(),
                                 info.level, info.depth,
                                 info.surface.image.get(),
                                 info.surface.mipmaps.get());
   }

   TCLFreeTilingAperture(handle);
   info.address = 0u;
   info.size = 0u;
}

namespace internal
{

// TODO: Remove this once we move to using physical addresses in GPU
bool
translateAperture(virt_addr &address,
                  uint32_t &size)
{
   for (auto i = 0u; i < sApertureData->apertureInfo.size(); ++i) {
      auto &info = sApertureData->apertureInfo[i];
      if (address >= info.address &&
          address < info.address + info.size) {
         if (info.level == 0) {
            size = info.surface.imageSize;
            address = virt_cast<virt_addr>(info.surface.image);
            return true;
         } else if (info.level >= 1) {
            size = info.surface.mipmapSize;
            address = virt_cast<virt_addr>(info.surface.mipmaps);
            return true;
         }
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
