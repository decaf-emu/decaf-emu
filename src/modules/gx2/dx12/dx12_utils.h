#pragma once
#include "dx12_state.h"
#include "modules/gx2/gx2_surface.h"
#include "utils/align.h"

static const DXGI_FORMAT dx12MakeFormat(GX2SurfaceFormat::Format format) {
   switch (format) {
   case GX2SurfaceFormat::UNORM_R8:
      return DXGI_FORMAT_R8_UNORM;
   case GX2SurfaceFormat::UNORM_R8G8B8A8:
      return DXGI_FORMAT_R8G8B8A8_UNORM;
   case GX2SurfaceFormat::UNORM_BC1:
      return DXGI_FORMAT_BC1_UNORM;
   case GX2SurfaceFormat::UNORM_BC2:
      return DXGI_FORMAT_BC3_UNORM;
   case GX2SurfaceFormat::UNORM_BC3:
      return DXGI_FORMAT_BC3_UNORM;
   case GX2SurfaceFormat::UNORM_BC4:
      return DXGI_FORMAT_BC4_UNORM;
   case GX2SurfaceFormat::UNORM_BC5:
      return DXGI_FORMAT_BC5_UNORM;
   default:
      throw;
   };
};

static const D3D12_BLEND dx12MakeBlend(GX2BlendMode::Mode mode) {
   switch (mode) {
   case GX2BlendMode::Zero:
      return D3D12_BLEND_ZERO;
   case  GX2BlendMode::One:
      return D3D12_BLEND_ONE;
   case  GX2BlendMode::SrcColor:
      return D3D12_BLEND_SRC_COLOR;
   case  GX2BlendMode::InvSrcColor:
      return D3D12_BLEND_INV_SRC_COLOR;
   case  GX2BlendMode::SrcAlpha:
      return D3D12_BLEND_SRC_ALPHA;
   case  GX2BlendMode::InvSrcAlpha:
      return D3D12_BLEND_INV_SRC_ALPHA;
   case  GX2BlendMode::DestAlpha:
      return D3D12_BLEND_DEST_ALPHA;
   case  GX2BlendMode::InvDestAlpha:
      return D3D12_BLEND_INV_DEST_ALPHA;
   case  GX2BlendMode::DestColor:
      return D3D12_BLEND_DEST_COLOR;
   case  GX2BlendMode::InvDestColor:
      return D3D12_BLEND_INV_DEST_COLOR;
   case  GX2BlendMode::SrcAlphaSat:
      return D3D12_BLEND_SRC_ALPHA_SAT;
   case  GX2BlendMode::BlendFactor:
      return D3D12_BLEND_BLEND_FACTOR;
   case  GX2BlendMode::InvBlendFactor:
      return D3D12_BLEND_INV_BLEND_FACTOR;
   case  GX2BlendMode::Src1Color:
      return D3D12_BLEND_SRC1_COLOR;
   case  GX2BlendMode::InvSrc1Color:
      return D3D12_BLEND_INV_SRC1_COLOR;
   case  GX2BlendMode::Src1Alpha:
      return D3D12_BLEND_SRC1_ALPHA;
   case  GX2BlendMode::InvSrc1Alpha:
      return D3D12_BLEND_INV_SRC1_ALPHA;
   default:
      throw;
   }
}

static const D3D12_PRIMITIVE_TOPOLOGY dx12MakePrimitiveTopology(GX2PrimitiveMode::Mode mode) {
   switch (mode) {
   case GX2PrimitiveMode::Triangles:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
   case GX2PrimitiveMode::TriangleStrip:
      return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

   case GX2PrimitiveMode::Quads: // Should be handled outside this method
   case GX2PrimitiveMode::QuadStrip: // Should be handled outside this method
   default:
      throw;
   }
}

static const D3D12_BLEND_OP dx12MakeBlendOp(GX2BlendCombineMode::Mode op) {
   switch (op) {
   case GX2BlendCombineMode::Add:
      return D3D12_BLEND_OP_ADD;
   case GX2BlendCombineMode::Subtract:
      return D3D12_BLEND_OP_SUBTRACT;
   case GX2BlendCombineMode::Min:
      return D3D12_BLEND_OP_MIN;
   case GX2BlendCombineMode::Max:
      return D3D12_BLEND_OP_MAX;
   case GX2BlendCombineMode::RevSubtract:
      return D3D12_BLEND_OP_REV_SUBTRACT;
   default:
      throw;
   }
}

static const uint32_t dx12FixSize(GX2SurfaceFormat::Format format, uint32_t size) {
   switch (format) {
   case GX2SurfaceFormat::UNORM_BC1:
   case GX2SurfaceFormat::UNORM_BC2:
   case GX2SurfaceFormat::UNORM_BC3:
   case GX2SurfaceFormat::UNORM_BC4:
   case GX2SurfaceFormat::UNORM_BC5:
      return align_up(size, 4);
   default:
      return size;
   }
}
