#pragma once
#include <array>
#include <addrlib/addrinterface.h>

namespace gpu7::tiling
{

struct SurfaceDescription
{
   AddrTileMode tileMode;
   AddrFormat format;
   uint32_t bpp;
   uint32_t numSamples;
   uint32_t width;
   uint32_t height;
   uint32_t numSlices;
   ADDR_SURFACE_FLAGS flags;
   uint32_t numFrags;
   uint32_t numLevels;
   uint32_t bankSwizzle;
   uint32_t pipeSwizzle;
};

ADDR_COMPUTE_SURFACE_INFO_OUTPUT
computeSurfaceInfo(const SurfaceDescription &surface,
                   int mipLevel,
                   int slice);

void
untileImage(const SurfaceDescription &surface,
            void *src,
            void *dst);

void
untileImageSlice(const SurfaceDescription &surface,
                 void *src,
                 void *dst,
                 int slice);

void
untileMipMap(const SurfaceDescription &desc,
             void *src,
             void *dst);

void
untileMip(const SurfaceDescription &surface,
          void *src,
          void *dst,
          int level);

void
untileMipSlice(const SurfaceDescription &desc,
               void *src,
               void *dst,
               int level,
               int slice);

size_t
computeUnpitchedImageSize(const SurfaceDescription &desc);

size_t
computeUnpitchedMipMapSize(const SurfaceDescription &desc);

void
unpitchImage(const SurfaceDescription &desc,
             void *pitched,
             void *unpitched);

void
unpitchMipMap(const SurfaceDescription &desc,
              void *pitched,
              void *unpitched);

} // namespace gpu7::tiling
