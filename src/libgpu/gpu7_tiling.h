#pragma once
#include <array>

namespace gpu7::tiling
{

enum class TileMode
{
   LinearGeneral  = 0,
   LinearAligned  = 1,
   Tiled1DThin1   = 2,
   Tiled1DThick   = 3,
   Tiled2DThin1   = 4,
   Tiled2DThin2   = 5,
   Tiled2DThin4   = 6,
   Tiled2DThick   = 7,
   Tiled2BThin1   = 8,
   Tiled2BThin2   = 9,
   Tiled2BThin4   = 10,
   Tiled2BThick   = 11,
   Tiled3DThin1   = 12,
   Tiled3DThick   = 13,
   Tiled3BThin1   = 14,
   Tiled3BThick   = 15,
};

struct SurfaceInfo
{
   int bpp = 0;
   TileMode tileMode = TileMode::LinearGeneral;
   int width = 0;
   int height = 0;
   int depth = 0;
   int numSamples = 1;
   bool isDepth = false;
   bool is3D = false;
   int bankSwizzle = 0;
   int pipeSwizzle = 0;
};

struct SurfaceMipMapInfo
{
   int numLevels = 0;
   int microTileLevel = 0;
   int size = 0;
   std::array<int, 13> offsets { };
};

int
calculateBaseAddressAlignment(const SurfaceInfo &surface);

int
calculateAlignedDepth(const SurfaceInfo &surface);

int
calculateAlignedHeight(const SurfaceInfo &surface);

int
calculateAlignedPitch(const SurfaceInfo &surface);

int
calculateImageSize(const SurfaceInfo &surface);

int
calculateSliceSize(const SurfaceInfo &surface);

int
calculateMipMapLevelSize(const SurfaceInfo &surface,
                         int level);

int
calculateMipMapLevelSliceSize(const SurfaceInfo &surface,
                              int level);

int
calculateUnpitchedImageSize(const SurfaceInfo &surface);

int
calculateUnpitchedSliceSize(const SurfaceInfo &surface);

void
calculateUnpitchedMipMapInfo(const SurfaceInfo &surface,
                             int numLevels,
                             SurfaceMipMapInfo &info);

void
calculateUnpitchedMipMapSliceInfo(const SurfaceInfo &surface,
                                  int numLevels,
                                  int slice,
                                  SurfaceMipMapInfo &info);

void
calculateMipMapInfo(const SurfaceInfo &surface,
                    int numLevels,
                    SurfaceMipMapInfo &info);

void
calculateMipMapSliceInfo(const SurfaceInfo &surface,
                         int numLevels,
                         int slice,
                         SurfaceMipMapInfo &info);

void
untileImage(const SurfaceInfo &surface,
            void *src,
            void *dst);

void
untileSlice(const SurfaceInfo &surface,
            void *src,
            void *dst,
            int slice,
            int sample = 0);

void
untileMipMaps(const SurfaceInfo &surface,
              const SurfaceMipMapInfo &mipMapInfo,
              void *src,
              void *dst);

void
untileMipMapsForSlice(const SurfaceInfo &surface,
                      const SurfaceMipMapInfo &mipMapInfo,
                      const SurfaceMipMapInfo &mipMapSliceInfo,
                      void *src,
                      void *dst,
                      int slice,
                      int sample = 0);

void
unpitchImage(const SurfaceInfo &surface,
             void *src,
             void *dst);

void
unpitchSlice(const SurfaceInfo &surface,
             void *src,
             void *dst);

void
unpitchMipMaps(const SurfaceInfo &surface,
               const SurfaceMipMapInfo &untiledMipMapInfo,
               const SurfaceMipMapInfo &unpitchedMipMapInfo,
               void *src,
               void *dst);

void
unpitchMipMapsForSlice(const SurfaceInfo &surface,
                       const SurfaceMipMapInfo &untiledMipMapInfo,
                       const SurfaceMipMapInfo &unpitchedMipMapInfo,
                       void *src,
                       void *dst,
                       int slice);

} // namespace gpu7::tiling
