#include "gx2.h"
#include "gx2_surface.h"
#include "gx2r_memory.h"
#include "gx2r_resource.h"
#include "gx2r_surface.h"
#include "cafe/libraries/coreinit/coreinit_cache.h"

namespace cafe::gx2
{

using namespace coreinit;

namespace internal
{

static void
getSurfaceData(virt_ptr<GX2Surface> surface,
               int32_t level,
               virt_ptr<void> *addr,
               uint32_t *size)
{
   if (level == 0) {
      *addr = surface->image;
      *size = surface->imageSize;
   } else if (level == -1) {
      *addr = surface->mipmaps;
      *size = surface->mipmapSize;
   } else {
      decaf_check(level > 0);
      auto curLevelOffset = 0u;
      auto nextLevelOffset = 0u;

      if (level > 1) {
         curLevelOffset = surface->mipLevelOffset[level - 1];
      }

      if (static_cast<uint32_t>(level + 1) >= surface->mipLevels) {
         nextLevelOffset = surface->mipmapSize;
      } else {
         nextLevelOffset = surface->mipLevelOffset[level];
      }

      *addr = surface->mipmaps + curLevelOffset;
      *size = nextLevelOffset - curLevelOffset;
   }
}

} // namespace internal

BOOL
GX2RCreateSurface(virt_ptr<GX2Surface> surface,
                  GX2RResourceFlags flags)
{
   surface->resourceFlags = flags;
   surface->resourceFlags &= ~GX2RResourceFlags::Locked;
   surface->resourceFlags |= GX2RResourceFlags::Gx2rAllocated;
   GX2CalcSurfaceSizeAndAlignment(surface);

   auto buffer = internal::gx2rAlloc(surface->resourceFlags,
                                     surface->imageSize + surface->mipmapSize,
                                     surface->alignment);

   surface->image = virt_cast<uint8_t *>(buffer);

   if (!surface->image) {
      return FALSE;
   }

   surface->mipmaps = nullptr;

   if (surface->mipmapSize) {
      surface->mipmaps = surface->image + surface->imageSize;
   }

   if ((surface->resourceFlags & GX2RResourceFlags::UsageGpuWrite) ||
      (surface->resourceFlags & GX2RResourceFlags::UsageDmaWrite)) {
      DCInvalidateRange(surface->image, surface->imageSize);
   }

   return TRUE;
}

BOOL
GX2RCreateSurfaceUserMemory(virt_ptr<GX2Surface> surface,
                            virt_ptr<uint8_t> image,
                            virt_ptr<uint8_t> mipmap,
                            GX2RResourceFlags flags)
{
   GX2CalcSurfaceSizeAndAlignment(surface);
   surface->resourceFlags = flags;
   surface->resourceFlags &= ~GX2RResourceFlags::Locked;
   surface->resourceFlags &= ~GX2RResourceFlags::Gx2rAllocated;
   surface->image = image;
   surface->mipmaps = mipmap;

   if ((surface->resourceFlags & GX2RResourceFlags::UsageGpuWrite) ||
      (surface->resourceFlags & GX2RResourceFlags::UsageDmaWrite)) {
      DCInvalidateRange(surface->image, surface->imageSize);

      if (surface->mipmaps) {
         DCInvalidateRange(surface->mipmaps, surface->mipmapSize);
      }
   }

   return true;
}

void
GX2RDestroySurfaceEx(virt_ptr<GX2Surface> surface,
                     GX2RResourceFlags flags)
{
   if (!surface || !surface->image) {
      return;
   }

   flags = surface->resourceFlags | internal::getOptionFlags(flags);

   if (!GX2RIsUserMemory(surface->resourceFlags)) {
      gx2::internal::gx2rFree(flags, surface->image);
   }

   surface->image = nullptr;
}

virt_ptr<void>
GX2RLockSurfaceEx(virt_ptr<GX2Surface> surface,
                  int32_t level,
                  GX2RResourceFlags flags)
{
   decaf_check(surface);
   decaf_check(surface->resourceFlags & ~GX2RResourceFlags::Locked);
   flags = surface->resourceFlags | internal::getOptionFlags(flags);

   // Set Locked flag
   surface->resourceFlags |= GX2RResourceFlags::Locked;
   surface->resourceFlags |= flags & GX2RResourceFlags::LockedReadOnly;

   // Check if we need to invalidate the surface.
   if ((flags & GX2RResourceFlags::UsageGpuWrite) ||
      (flags & GX2RResourceFlags::UsageDmaWrite)) {
      if (!(flags & GX2RResourceFlags::DisableCpuInvalidate)) {
         auto ptr = virt_ptr<void> { nullptr };
         auto size = uint32_t { 0 };
         internal::getSurfaceData(surface, level, &ptr, &size);
         DCInvalidateRange(ptr, size);
      }
   }

   return surface->image;
}

void
GX2RUnlockSurfaceEx(virt_ptr<GX2Surface> surface,
                    int32_t level,
                    GX2RResourceFlags flags)
{
   decaf_check(surface);
   decaf_check(surface->resourceFlags & GX2RResourceFlags::Locked);

   // Invalidate surface
   GX2RInvalidateSurface(surface, level, flags);

   // Clear locked flags.
   surface->resourceFlags &= ~GX2RResourceFlags::LockedReadOnly;
   surface->resourceFlags &= ~GX2RResourceFlags::Locked;
}

BOOL
GX2RIsGX2RSurface(GX2RResourceFlags flags)
{
   return (flags & (GX2RResourceFlags::UsageCpuReadWrite | GX2RResourceFlags::UsageGpuReadWrite)) ? TRUE : FALSE;
}

void
GX2RInvalidateSurface(virt_ptr<GX2Surface> surface,
                      int32_t level,
                      GX2RResourceFlags flags)
{
   flags = internal::getOptionFlags(flags);

   // Get surface ptr & size
   auto ptr = virt_ptr<void> { nullptr };
   auto size = uint32_t { 0 };
   internal::getSurfaceData(surface, level, &ptr, &size);

   // Invalidate!
   GX2RInvalidateMemory(surface->resourceFlags | flags, ptr, size);
}

void
Library::registerGx2rSurfaceSymbols()
{
   RegisterFunctionExport(GX2RCreateSurface);
   RegisterFunctionExport(GX2RCreateSurfaceUserMemory);
   RegisterFunctionExport(GX2RDestroySurfaceEx);
   RegisterFunctionExport(GX2RLockSurfaceEx);
   RegisterFunctionExport(GX2RUnlockSurfaceEx);
   RegisterFunctionExport(GX2RIsGX2RSurface);
   RegisterFunctionExport(GX2RInvalidateSurface);
}

} // namespace cafe::gx2
