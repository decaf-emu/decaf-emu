#pragma once
#ifdef DECAF_VULKAN
#include "latte/latte_formats.h"
#include "latte/latte_constants.h"
#include "latte/latte_registers_pa.h"
#include "latte/latte_registers_sq.h"
#include "latte/latte_registers_vgt.h"

#include <common/datahash.h>
#include <libcpu/be2_struct.h>
#include <vulkan/vulkan.hpp>

namespace vulkan
{

struct RenderPassObject;
struct VertexShaderObject;
struct GeometryShaderObject;
struct PixelShaderObject;
struct RectStubShaderObject;

#pragma pack(push, 1)

// This is a specialized class for holding floats inside our hashable structures.
// This allows us to pull float data out of our registers and store it in our
// description structures without breaking the hashability required by DataHash.
struct hashableFloat
{
   static_assert(sizeof(float) == sizeof(uint32_t),
                 "hashable float implementation requires uint32_t and float to be the same size");

   hashableFloat()
      : value(0)
   {
   }

   hashableFloat(float val)
      : value(*reinterpret_cast<uint32_t*>(&val))
   {
   }

   operator float() const
   {
      return *reinterpret_cast<const float*>(&value);
   }

   float & operator=(const float & rhs)
   {
      return *reinterpret_cast<float*>(&value) = rhs;
   }

   uint32_t value;
};

struct ColorBufferDesc
{
   uint32_t base256b;
   uint32_t pitchTileMax;
   uint32_t sliceTileMax;
   latte::CB_FORMAT format;
   latte::CB_NUMBER_TYPE numberType;
   latte::BUFFER_ARRAY_MODE arrayMode;
   uint32_t sliceStart;
   uint32_t sliceEnd;
};

struct DepthStencilBufferDesc
{
   uint32_t base256b;
   uint32_t pitchTileMax;
   uint32_t sliceTileMax;
   latte::DB_FORMAT format;
   latte::BUFFER_ARRAY_MODE arrayMode;
   uint32_t sliceStart;
   uint32_t sliceEnd;
};

struct VertexBufferDesc
{
   phys_addr baseAddress;
   uint32_t size;
   uint32_t stride;
};

struct SurfaceDesc
{
   // BaseAddress is a uint32 rather than a phys_addr as it actually
   // encodes information other than just the base address (swizzle).

   uint32_t baseAddress;
   uint32_t pitch;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t samples;
   latte::SQ_TEX_DIM dim;
   latte::SQ_TILE_TYPE tileType;
   latte::SQ_TILE_MODE tileMode;
   latte::SurfaceFormat format;

   inline uint32_t calcAlignedBaseAddress() const
   {
      if (tileMode >= latte::SQ_TILE_MODE::TILED_2D_THIN1) {
         return baseAddress & ~(0x800 - 1);
      } else {
         return baseAddress & ~(0x100 - 1);
      }
   }

   inline uint32_t calcSwizzle() const
   {
      return baseAddress & 0x00000F00;
   }

   inline DataHash hash(bool byCompat = false) const
   {
      // tileMode and swizzle are intentionally omited as they
      // do not affect data placement or size, but only upload/downloads.
      // It is possible that a tile-type switch may invalidate
      // old data though...

      // TODO: Handle tiling changes, major memory reuse issues...

      struct
      {
         uint32_t address;
         uint32_t format;
         uint32_t dim;
         uint32_t samples;
         uint32_t tileType;
         uint32_t tileMode;
         uint32_t width;
         uint32_t pitch;
         uint32_t height;
         uint32_t depth;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.address = calcAlignedBaseAddress();
      _dataHash.format = format;
      _dataHash.samples = samples;
      _dataHash.tileType = tileType;
      _dataHash.tileMode = tileMode;

      if (!byCompat) {
         // TODO: Figure out if we can emulate 2D_ARRAY surfaces
         // as 3D surfaces so we can get both kinds of views from
         // them?
         // Figure out which surfaces are compatible
         // at a DIM level...  Basically, anything we
         // can generate a view of.
         switch (dim) {
         case latte::SQ_TEX_DIM::DIM_3D:
            _dataHash.dim = 3;
            break;
         case latte::SQ_TEX_DIM::DIM_2D:
         case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
         case latte::SQ_TEX_DIM::DIM_CUBEMAP:
            _dataHash.dim = 2;
            break;
         case latte::SQ_TEX_DIM::DIM_1D:
         case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
            _dataHash.dim = 1;
            break;
         default:
            decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
         }
         //_dataHash.dim = dim;
      }

      switch (dim) {
      case latte::SQ_TEX_DIM::DIM_3D:
         if (!byCompat) {
            _dataHash.depth = depth;
         }
         // fallthrough
      case latte::SQ_TEX_DIM::DIM_2D:
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         _dataHash.height = height;
         // fallthrough
      case latte::SQ_TEX_DIM::DIM_1D:
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
         _dataHash.pitch = pitch;
         if (!byCompat) {
            _dataHash.width = width;
         }
         break;
      default:
         decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
      }

      return DataHash {}.write(_dataHash);
   }
};

struct SurfaceViewDesc
{
   SurfaceDesc surfaceDesc;
   uint32_t sliceStart;
   uint32_t sliceEnd;
   std::array<latte::SQ_SEL, 4> channels;

   inline DataHash hash() const
   {
      struct
      {
         uint32_t sliceStart;
         uint32_t sliceEnd;
         std::array<latte::SQ_SEL, 4> channels;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.sliceStart = sliceStart;
      _dataHash.sliceEnd = sliceEnd;
      _dataHash.channels = channels;

      return surfaceDesc.hash().write(_dataHash);
   }
};

struct FramebufferDesc
{
   std::array<ColorBufferDesc, latte::MaxRenderTargets> colorTargets;
   DepthStencilBufferDesc depthTarget;

   inline DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct SamplerDesc
{
   latte::SQ_TEX_SAMPLER_WORD0_N texSamplerWord0;
   latte::SQ_TEX_SAMPLER_WORD1_N texSamplerWord1;
   latte::SQ_TEX_SAMPLER_WORD2_N texSamplerWord2;

   inline DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct SwapChainDesc
{
   phys_addr baseAddress;
   uint32_t width;
   uint32_t height;
};

struct RenderPassDesc
{
   struct ColorTarget
   {
      bool isEnabled;
      latte::CB_FORMAT format;
      latte::CB_NUMBER_TYPE numberType;
      uint32_t samples;
   };

   struct DepthStencilTarget
   {
      bool isEnabled;
      latte::DB_FORMAT format;
   };

   std::array<ColorTarget, 8> colorTargets;
   DepthStencilTarget depthTarget;

   inline DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct PipelineLayoutDesc
{
   std::array<bool, latte::MaxSamplers> vsSamplerUsed;
   std::array<bool, latte::MaxTextures> vsTextureUsed;
   std::array<bool, latte::MaxUniformBlocks> vsBufferUsed;
   std::array<bool, latte::MaxSamplers> gsSamplerUsed;
   std::array<bool, latte::MaxTextures> gsTextureUsed;
   std::array<bool, latte::MaxUniformBlocks> gsBufferUsed;
   std::array<bool, latte::MaxSamplers> psSamplerUsed;
   std::array<bool, latte::MaxTextures> psTextureUsed;
   std::array<bool, latte::MaxUniformBlocks> psBufferUsed;

   uint32_t numDescriptors = 0;

   inline DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct PipelineDesc
{
   RenderPassObject *renderPass;
   VertexShaderObject *vertexShader;
   GeometryShaderObject *geometryShader;
   PixelShaderObject *pixelShader;
   RectStubShaderObject *rectStubShader;

   std::array<uint32_t, latte::MaxAttribBuffers> attribBufferStride;
   std::array<uint32_t, 2> attribBufferDivisor;
   latte::VGT_DI_PRIMITIVE_TYPE primitiveType;
   bool primitiveResetEnabled;
   uint32_t primitiveResetIndex;
   bool dx9Consts;

   struct StencilOpState
   {
      latte::REF_FUNC compareFunc;
      latte::DB_STENCIL_FUNC failOp;
      latte::DB_STENCIL_FUNC zPassOp;
      latte::DB_STENCIL_FUNC zFailOp;
      uint8_t ref;
      uint8_t mask;
      uint8_t writeMask;
   };
   bool stencilEnable;
   StencilOpState stencilFront;
   StencilOpState stencilBack;

   bool zEnable;
   bool zWriteEnable;
   latte::REF_FUNC zFunc;
   bool rasteriserDisable;
   uint32_t lineWidth;
   latte::PA_FACE paFace;
   bool cullFront;
   bool cullBack;
   uint32_t polyPType;
   bool polyBiasEnabled;
   hashableFloat polyBiasClamp;
   hashableFloat polyBiasOffset;
   hashableFloat polyBiasScale;
   bool zclipDisabled;

   uint32_t rop3;

   struct BlendControl
   {
      uint8_t targetMask;
      bool blendingEnabled;
      bool opacityWeight;
      latte::CB_COMB_FUNC colorCombFcn;
      latte::CB_BLEND_FUNC colorSrcBlend;
      latte::CB_BLEND_FUNC colorDstBlend;
      latte::CB_COMB_FUNC alphaCombFcn;
      latte::CB_BLEND_FUNC alphaSrcBlend;
      latte::CB_BLEND_FUNC alphaDstBlend;
   };

   std::array<BlendControl, latte::MaxRenderTargets> cbBlendControls;
   std::array<hashableFloat, 4> cbBlendConstants;

   latte::REF_FUNC alphaFunc;
   hashableFloat alphaRef;

   inline DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct StreamOutBufferDesc
{
   phys_addr baseAddress;
   uint32_t size;
   uint32_t stride;
};

#pragma pack(pop)

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
