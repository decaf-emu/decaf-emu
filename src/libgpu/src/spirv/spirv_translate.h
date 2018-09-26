#pragma optimize("", off)
#pragma once

#include "latte/latte_constants.h"
#include "latte/latte_registers_sq.h"
#include "latte/latte_registers_spi.h"

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
      return DataHash {}
         .write(type)
         .write(binary.data(), binary.size())
         .write(aluInstPreferVector);
   }
};

struct VertexShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> fsBinary;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   std::array<uint32_t, 2> instanceStepRates;

   struct {
      latte::SQ_PGM_RESOURCES_VS sq_pgm_resources_vs;
      std::array<latte::SQ_VTX_SEMANTIC_N, 32> sq_vtx_semantics;
      latte::SPI_VS_OUT_CONFIG spi_vs_out_config;
      std::array<latte::SPI_VS_OUT_ID_N, 10> spi_vs_out_ids;
   } regs;

   DataHash hash() const
   {
      return ShaderDesc::hash()
         .write(fsBinary.data(), fsBinary.size())
         .write(texDims)
         .write(instanceStepRates)
         .write(regs);
   }
};

struct GeometryShaderDesc : public ShaderDesc
{
   gsl::span<const uint8_t> dcBinary;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;

   DataHash hash() const
   {
      return ShaderDesc::hash()
         .write(dcBinary.data(), dcBinary.size())
         .write(texDims);
   }
};

struct PixelShaderDesc : public ShaderDesc
{
   std::vector<uint32_t> vsOutputSemantics;
   std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
   latte::REF_FUNC alphaRefFunc;

   struct {
      latte::SQ_PGM_RESOURCES_PS sq_pgm_resources_ps;
      latte::SQ_PGM_EXPORTS_PS sq_pgm_exports_ps;
      latte::SPI_PS_IN_CONTROL_0 spi_ps_in_control_0;
      latte::SPI_PS_IN_CONTROL_1 spi_ps_in_control_1;
      std::array<latte::SPI_PS_INPUT_CNTL_N, 32> spi_ps_input_cntls;
   } regs;

   DataHash hash() const
   {
      return ShaderDesc::hash()
         .write(vsOutputSemantics)
         .write(texDims)
         .write(alphaRefFunc)
         .write(regs);
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
   std::vector<uint32_t> outputSemantics;
};

struct GeometryShader : public Shader
{
};

struct PixelShader : public Shader
{
};

bool
translate(const ShaderDesc& shaderDesc, Shader *shader);

std::string
shaderToString(const Shader *shader);

} // namespace spirv
