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

struct InputBuffer
{
   enum class IndexMode : uint32_t
   {
      PerVertex,
      PerInstance,
   };

   bool isUsed;
   IndexMode indexMode;
   uint32_t divisor;
};

struct InputAttrib
{
   uint32_t bufferIndex;
   uint32_t offset;
   uint32_t elemWidth;
   uint32_t elemCount;
};

struct ShaderDesc
{
   ShaderType type = ShaderType::Unknown;
   gsl::span<const uint8_t> binary;
   bool aluInstPreferVector;

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct VertexShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> fsBinary;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   std::array<bool, latte::MaxTextures> texIsUint;
   std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;
   std::array<uint32_t, 2> instanceStepRates;
   bool generateRectStub;

   struct {
      latte::SQ_PGM_RESOURCES_VS sq_pgm_resources_vs;
      std::array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantics;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_ids;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
   } regs;

   VertexShaderDesc()
   {
      memset(this, 0, sizeof(*this));
   }

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct GeometryShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> dcBinary;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   std::array<bool, latte::MaxTextures> texIsUint;
   std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;

   struct
   {
      latte::SQ_GS_VERT_ITEMSIZE sq_gs_vert_itemsize;
      latte::VGT_GS_OUT_PRIMITIVE_TYPE vgt_gs_out_prim_type;
      latte::VGT_GS_MODE vgt_gs_mode;
      uint32_t sq_gsvs_ring_itemsize;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_ids;
      latte::PA_CL_VS_OUT_CNTL pa_cl_vs_out_cntl;
   } regs;

   GeometryShaderDesc()
   {
      memset(this, 0, sizeof(*this));
   }

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

enum ColorOutputType : uint32_t
{
   FLOAT,
   SINT,
   UINT
};

struct PixelShaderDesc : public ShaderDesc
{
   std::array<uint8_t, 40> vsOutputSemantics;
   std::array<ColorOutputType, latte::MaxRenderTargets> pixelOutType;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   std::array<bool, latte::MaxTextures> texIsUint;
   latte::REF_FUNC alphaRefFunc;

   struct {
      latte::SQ_PGM_RESOURCES_PS sq_pgm_resources_ps;
      latte::SQ_PGM_EXPORTS_PS sq_pgm_exports_ps;
      latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
      latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
      std::array<latte::SPI_PS_INPUT_CNTL_N, 32> spi_ps_input_cntls;
      latte::CB_SHADER_CONTROL cb_shader_control;
      latte::CB_SHADER_MASK cb_shader_mask;
      latte::DB_SHADER_CONTROL db_shader_control;
   } regs;

   PixelShaderDesc()
   {
      memset(this, 0, sizeof(*this));
   }

   DataHash hash() const
   {
      return DataHash {}.write(*this);
   }
};

struct Shader
{
   std::vector<unsigned int> binary;

   std::array<bool, latte::MaxSamplers> samplerUsed;
   std::array<bool, latte::MaxTextures> textureUsed;
   std::array<bool, latte::MaxUniformBlocks> cbufferUsed;
};

struct VertexShader : public Shader
{
   std::array<InputBuffer, latte::MaxAttribBuffers> inputBuffers;
   std::vector<InputAttrib> inputAttribs;
   std::array<uint8_t, 40> outputSemantics;

   std::vector<unsigned int> rectStubBinary;
};

struct GeometryShader : public Shader
{
   std::array<uint8_t, 40> outputSemantics;
};

struct PixelShader : public Shader
{
};

bool
translate(const ShaderDesc& shaderDesc, Shader *shader);

std::string
shaderToString(const Shader *shader);

} // namespace spirv

#endif // ifdef DECAF_VULKAN
