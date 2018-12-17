#pragma once
#ifdef DECAF_VULKAN

#include "latte/latte_constants.h"
#include "latte/latte_registers_cb.h"
#include "latte/latte_registers_db.h"
#include "latte/latte_registers_sq.h"
#include "latte/latte_registers_spi.h"
#include "latte/latte_registers_pa.h"
#include "latte/latte_registers_vgt.h"

#include <common/datahash.h>
#include <gsl/gsl>

namespace spirv
{

enum class ShaderType : uint32_t
{
   Unknown,
   Vertex,
   Geometry,
   Pixel
};

enum class TextureInputType : uint32_t
{
   NONE,
   FLOAT,
   INT
};

enum class PixelOutputType : uint32_t
{
   FLOAT,
   SINT,
   UINT
};

struct AttribBuffer
{
   enum class IndexMode : uint32_t
   {
      PerVertex,
      PerInstance,
   };

   enum class DivisorMode : uint32_t
   {
      CONST_1,
      REGISTER_0,
      REGISTER_1
   };

   bool isUsed = false;
   IndexMode indexMode = IndexMode::PerVertex;
   DivisorMode divisorMode = DivisorMode::CONST_1;
};

struct AttribElem
{
   uint32_t bufferIndex = 0;
   uint32_t offset = 0;
   uint32_t elemWidth = 0;
   uint32_t elemCount = 0;
};

#pragma pack(push, 1)

struct ShaderDesc
{
   ShaderType type = ShaderType::Unknown;
   gsl::span<const uint8_t> binary;
   bool aluInstPreferVector = true;

   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   std::array<TextureInputType, latte::MaxTextures> texFormat;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct VertexShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> fsBinary;

   std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;

   struct
   {
      latte::SQ_PGM_RESOURCES_VS sq_pgm_resources_vs;
      std::array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantics;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
   } regs;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct GeometryShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> dcBinary;

   std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;

   struct
   {
      latte::SQ_GS_VERT_ITEMSIZE sq_gs_vert_itemsize;
      latte::VGT_GS_OUT_PRIMITIVE_TYPE vgt_gs_out_prim_type;
      latte::VGT_GS_MODE vgt_gs_mode;
      uint32_t sq_gsvs_ring_itemsize;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
   } regs;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct PixelShaderDesc : public ShaderDesc
{
   std::array<PixelOutputType, latte::MaxRenderTargets> pixelOutType;

   struct
   {
      latte::SQ_PGM_RESOURCES_PS sq_pgm_resources_ps;
      latte::SQ_PGM_EXPORTS_PS sq_pgm_exports_ps;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_ids;
      latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
      latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
      std::array<latte::SPI_PS_INPUT_CNTL_N, 32> spi_ps_input_cntls;
      latte::CB_SHADER_CONTROL cb_shader_control;
      latte::CB_SHADER_MASK cb_shader_mask;
      latte::DB_SHADER_CONTROL db_shader_control;
   } regs;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

#pragma pack(pop)

struct RectStubShaderDesc
{
   uint32_t numVsExports;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct ShaderMeta
{
   std::array<bool, latte::MaxSamplers> samplerUsed;
   std::array<bool, latte::MaxTextures> textureUsed;
   std::array<bool, latte::MaxUniformBlocks> cbufferUsed;
   uint32_t cfileUsed;
};

struct VertexShaderMeta : public ShaderMeta
{
   uint32_t numExports;
   std::array<bool, latte::MaxStreamOutBuffers> streamOutUsed;
   std::array<AttribBuffer, latte::MaxAttribBuffers> attribBuffers;
   std::vector<AttribElem> attribElems;
};

struct GeometryShaderMeta : public ShaderMeta
{
   std::array<bool, latte::MaxStreamOutBuffers> streamOutUsed;
};

struct PixelShaderMeta : public ShaderMeta
{
   std::array<bool, latte::MaxRenderTargets> pixelOutUsed;
};

struct Shader
{
   std::vector<unsigned int> binary;
};

struct VertexShader : public Shader
{
   VertexShaderMeta meta;
};

struct GeometryShader : public Shader
{
   GeometryShaderMeta meta;
};

struct PixelShader : public Shader
{
   PixelShaderMeta meta;
};

struct RectStubShader
{
   std::vector<unsigned int> binary;
};

bool
translate(const ShaderDesc& shaderDesc, Shader *shader);

RectStubShaderDesc
generateRectSubShaderDesc(VertexShader *vertexShader);

bool
generateRectStub(const RectStubShaderDesc& shaderDesc, RectStubShader *shader);

std::string
shaderToString(const Shader *shader);

} // namespace spirv

#endif // ifdef DECAF_VULKAN
