#pragma once
#include "dx12_state.h"
#include "../gx2_surface.h"

static const DXGI_FORMAT dx12MakeFormat(GX2SurfaceFormat::Format format) {
   switch (format) {
   case GX2SurfaceFormat::UNORM_R8G8B8A8:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case GX2SurfaceFormat::UNORM_BC1:
      return DXGI_FORMAT_BC1_UNORM;
   case GX2SurfaceFormat::UNORM_BC3:
      return DXGI_FORMAT_BC3_UNORM;
   default:
      assert(0);
      return DXGI_FORMAT_UNKNOWN;
   };
};

static const uint32_t dx12FixSize(GX2SurfaceFormat::Format format, uint32_t size) {
   switch (format) {
   case GX2SurfaceFormat::UNORM_BC1:
      return (size >> 2) * 4;
   case GX2SurfaceFormat::UNORM_BC3:
      return (size >> 2) * 4;
   default:
      return size;
   }
}
