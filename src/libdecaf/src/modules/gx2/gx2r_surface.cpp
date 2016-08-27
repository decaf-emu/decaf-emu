#include "gx2r_resource.h"
#include "gx2r_surface.h"
#include "gx2r_mem.h"
#include "gx2_surface.h"

namespace gx2
{

namespace internal
{

static void
getSurfaceData(GX2Surface *surface,
               int32_t level,
               uint32_t *addr,
               uint32_t *size)
{
   if (level == 0) {
      *addr = surface->image.getAddress();
      *size = surface->imageSize;
   } else if (level == -1) {
      *addr = surface->mipmaps.getAddress();
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

      *addr = surface->mipmaps.getAddress() + curLevelOffset;
      *size = nextLevelOffset - curLevelOffset;
   }
}

} // namespace internal

bool
GX2RCreateSurface(GX2Surface *surface,
                  GX2RResourceFlags flags)
{
   surface->resourceFlags = flags;
   surface->resourceFlags |= GX2RResourceFlags::UsageCpuReadWrite;
   surface->resourceFlags |= GX2RResourceFlags::UsageGpuReadWrite;
   GX2CalcSurfaceSizeAndAlignment(surface);

   auto buffer = gx2::internal::gx2rAlloc(surface->resourceFlags,
                                          surface->imageSize + surface->mipmapSize,
                                          surface->alignment);

   surface->image = reinterpret_cast<uint8_t *>(buffer);

   if (buffer) {
      surface->mipmaps = nullptr;

      if (surface->mipmapSize) {
         surface->mipmaps = surface->image + surface->imageSize;
      }
   }

   return !!buffer;
}

bool
GX2RCreateSurfaceUserMemory(GX2Surface *surface,
                            uint8_t *image,
                            uint8_t *mipmap,
                            GX2RResourceFlags flags)
{
   GX2CalcSurfaceSizeAndAlignment(surface);
   surface->resourceFlags = flags;
   surface->resourceFlags |= GX2RResourceFlags::UsageCpuReadWrite;
   surface->resourceFlags |= GX2RResourceFlags::UsageGpuReadWrite;
   surface->image = image;
   surface->mipmaps = mipmap;
   return true;
}

void
GX2RDestroySurfaceEx(GX2Surface *surface,
                     GX2RResourceFlags flags)
{
   if (!surface || !surface->image)
      return;

   flags = surface->resourceFlags | flags;

   if (!GX2RIsUserMemory(surface->resourceFlags)) {
      gx2::internal::gx2rFree(flags, surface->image);
   }

   surface->image = nullptr;
   surface->mipmaps = nullptr;
}

void *
GX2RLockSurfaceEx(GX2Surface *surface,
                  int32_t level,
                  GX2RResourceFlags flags)
{
   // Allow flags in bits 19 to 23
   flags = static_cast<GX2RResourceFlags>(flags & 0xF80000);

   // Update buffer flags
   surface->resourceFlags |= flags | GX2RResourceFlags::Locked;

   // Return buffer image
   uint32_t addr, size;
   internal::getSurfaceData(surface, level, &addr, &size);
   return mem::translate(addr);
}

void
GX2RUnlockSurfaceEx(GX2Surface *surface,
                    int32_t level,
                    GX2RResourceFlags flags)
{
   // Allow flags in bits 19 to 23
   flags = static_cast<GX2RResourceFlags>(flags & 0xF80000);

   // Invalidate the GPU surface only if it was not read only locked
   if (!(surface->resourceFlags & GX2RResourceFlags::LockedReadOnly)) {
      GX2RInvalidateSurface(surface, level, flags);
   }

   // Update buffer flags
   surface->resourceFlags &= ~GX2RResourceFlags::LockedReadOnly & ~GX2RResourceFlags::Locked;
}

BOOL
GX2RIsGX2RSurface(GX2RResourceFlags flags)
{
   return (flags & (GX2RResourceFlags::UsageCpuReadWrite | GX2RResourceFlags::UsageGpuReadWrite)) ? TRUE : FALSE;
}

void
GX2RInvalidateSurface(GX2Surface *surface,
                      int32_t level,
                      GX2RResourceFlags flags)
{
   // Allow flags in bits 19 to 23
   flags = static_cast<GX2RResourceFlags>(flags & 0xF80000);

   // Get surface addr & size
   uint32_t addr, size;
   internal::getSurfaceData(surface, level, &addr, &size);

   // Invalidate!
   GX2RInvalidateMemory(surface->resourceFlags | flags,
                        mem::translate(addr),
                        size);
}

} // namespace gx2
