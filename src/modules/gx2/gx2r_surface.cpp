#include "gx2r_resource.h"
#include "gx2r_surface.h"
#include "gx2_surface.h"

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
GX2RDestroySurfaceEx(GX2Surface *surface, GX2RResourceFlags flags)
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
GX2RLockSurfaceEx(GX2Surface *surface, int32_t level, GX2RResourceFlags flags)
{
   surface->resourceFlags |= GX2RResourceFlags::Locked;
   return surface->image;
}

void
GX2RUnlockSurfaceEx(GX2Surface *surface, int32_t level, GX2RResourceFlags flags)
{
   surface->resourceFlags &= ~GX2RResourceFlags::Locked;
}

BOOL
GX2RIsGX2RSurface(GX2RResourceFlags flags)
{
   return (flags & (GX2RResourceFlags::UsageCpuReadWrite | GX2RResourceFlags::UsageGpuReadWrite)) ? TRUE : FALSE;
}
