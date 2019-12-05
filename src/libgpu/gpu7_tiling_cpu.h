#pragma once
#include "gpu7_tiling.h"

namespace gpu7::tiling::cpu
{

void
untileImage(const SurfaceDescription& surface,
            void* src,
            void* dst);

void
untileImageSlice(const SurfaceDescription& surface,
                 void* src,
                 void* dst,
                 int slice);

void
untileMipMap(const SurfaceDescription& desc,
             void* src,
             void* dst);

void
untileMip(const SurfaceDescription& surface,
          void* src,
          void* dst,
          int level);

void
untileMipSlice(const SurfaceDescription& desc,
               void* src,
               void* dst,
               int level,
               int slice);

}  // namespace gpu7::tiling::cpu
