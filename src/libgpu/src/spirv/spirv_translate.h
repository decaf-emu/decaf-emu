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
      struct {
         ShaderType type;
         uint32_t preferVector;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.type = type;
      _dataHash.preferVector = aluInstPreferVector ? 1 : 0;

      return DataHash {}
         .write(binary.data(), binary.size())
         .write(_dataHash);
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

   DataHash hash() const
   {
      struct
      {
         std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
         std::array<bool, latte::MaxTextures> texIsUint;
         std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;
         std::array<uint32_t, 2> instanceStepRates;
         bool generateRectStub;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.texDims = texDims;
      _dataHash.texIsUint = texIsUint;
      _dataHash.streamOutStride = streamOutStride;
      _dataHash.instanceStepRates = instanceStepRates;
      _dataHash.generateRectStub = generateRectStub;

      return ShaderDesc::hash()
         .write(fsBinary.data(), fsBinary.size())
         .write(_dataHash)
         .write(regs);
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

   DataHash hash() const
   {
      struct
      {
         std::array<latte::SQ_TEX_DIM, latte::MaxTextures> texDims;
         std::array<bool, latte::MaxTextures> texIsUint;
         std::array<uint32_t, latte::MaxStreamOutBuffers> streamOutStride;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.texDims = texDims;
      _dataHash.texIsUint = texIsUint;
      _dataHash.streamOutStride = streamOutStride;

      return ShaderDesc::hash()
         .write(dcBinary.data(), dcBinary.size())
         .write(_dataHash)
         .write(regs);
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
   std::vector<uint32_t> vsOutputSemantics;
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

   DataHash hash() const
   {
      return ShaderDesc::hash()
         .write(vsOutputSemantics)
         .write(pixelOutType)
         .write(texDims)
         .write(texIsUint)
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

   std::vector<unsigned int> rectStubBinary;
};

struct GeometryShader : public Shader
{
   std::vector<uint32_t> outputSemantics;
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
