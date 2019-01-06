#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_config.h"
#include "latte/latte_disassembler.h"

#include <common/log.h>
#include <common/platform_dir.h>
#include <fstream>
#include <vector>

namespace vulkan
{

spirv::TextureInputType
spirvTextureTypeFromLatte(latte::SQ_NUM_FORMAT format)
{
   if (format == latte::SQ_NUM_FORMAT::INT) {
      return spirv::TextureInputType::INT;
   }
   return spirv::TextureInputType::FLOAT;
}

spirv::PixelOutputType
spirvPixelTypeFromLatte(latte::CB_NUMBER_TYPE format)
{
   switch (format) {
   case latte::CB_NUMBER_TYPE::SINT:
      return spirv::PixelOutputType::SINT;
   case latte::CB_NUMBER_TYPE::UINT:
      return spirv::PixelOutputType::UINT;
   default:
      return spirv::PixelOutputType::FLOAT;
   }
}

spirv::VertexShaderDesc
Driver::getVertexShaderDesc()
{
   gsl::span<uint8_t> fsShaderBinary;
   gsl::span<uint8_t> vsShaderBinary;

   auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
   auto pgm_offset_fs = getRegister<latte::SQ_PGM_CF_OFFSET_FS>(latte::Register::SQ_PGM_CF_OFFSET_FS);
   auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
   fsShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_fs.PGM_START() << 8)).getRawPointer(),
      pgm_size_fs.PGM_SIZE() << 3);
   decaf_check(pgm_offset_fs.PGM_OFFSET() == 0);

   auto vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   if (vgt_gs_mode.MODE() == latte::VGT_GS_ENABLE_MODE::OFF) {
      // When GS is disabled, vertex shader comes from vertex shader register
      auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
      auto pgm_offset_vs = getRegister<latte::SQ_PGM_CF_OFFSET_VS>(latte::Register::SQ_PGM_CF_OFFSET_VS);
      auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
      vsShaderBinary = gsl::make_span(
         phys_cast<uint8_t*>(phys_addr(pgm_start_vs.PGM_START() << 8)).getRawPointer(),
         pgm_size_vs.PGM_SIZE() << 3);
      decaf_check(pgm_offset_vs.PGM_OFFSET() == 0);
   } else {
      // When GS is enabled, vertex shader comes from export shader register
      auto pgm_start_es = getRegister<latte::SQ_PGM_START_ES>(latte::Register::SQ_PGM_START_ES);
      auto pgm_offset_es = getRegister<latte::SQ_PGM_CF_OFFSET_ES>(latte::Register::SQ_PGM_CF_OFFSET_ES);
      auto pgm_size_es = getRegister<latte::SQ_PGM_SIZE_ES>(latte::Register::SQ_PGM_SIZE_ES);

      vsShaderBinary = gsl::make_span(
         phys_cast<uint8_t*>(phys_addr(pgm_start_es.PGM_START() << 8)).getRawPointer(),
         pgm_size_es.PGM_SIZE() << 3);
      decaf_check(pgm_offset_es.PGM_OFFSET() == 0);
   }

   auto shaderDesc = spirv::VertexShaderDesc { };
   shaderDesc.type = spirv::ShaderType::Vertex;
   shaderDesc.binary = vsShaderBinary;
   shaderDesc.fsBinary = fsShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texFormat[i] = spirvTextureTypeFromLatte(sq_tex_resource_word4.NUM_FORMAT_ALL());
   }

   shaderDesc.regs.sq_pgm_resources_vs = getRegister<latte::SQ_PGM_RESOURCES_VS>(latte::Register::SQ_PGM_RESOURCES_VS);
   shaderDesc.regs.pa_cl_vs_out_cntl = getRegister<latte::PA_CL_VS_OUT_CNTL>(latte::Register::PA_CL_VS_OUT_CNTL);

   for (auto i = 0u; i < 32; ++i) {
      shaderDesc.regs.sq_vtx_semantics[i] = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
   }

   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      // Note that these registers are not contiguous!
      shaderDesc.streamOutStride[i] = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + i * 16) << 2;
   }

   return shaderDesc;
}

spirv::GeometryShaderDesc
Driver::getGeometryShaderDesc()
{
   // Do not generate geometry shaders if they are disabled
   auto vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   if (vgt_gs_mode.MODE() == latte::VGT_GS_ENABLE_MODE::OFF) {
      return spirv::GeometryShaderDesc();
   }

   decaf_check(mCurrentDraw->vertexShader);

   // Geometry shader comes from geometry shader register
   auto pgm_start_gs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_GS);
   auto pgm_offset_gs = getRegister<latte::SQ_PGM_CF_OFFSET_GS>(latte::Register::SQ_PGM_CF_OFFSET_GS);
   auto pgm_size_gs = getRegister<latte::SQ_PGM_SIZE_GS>(latte::Register::SQ_PGM_SIZE_GS);
   auto gsShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_gs.PGM_START() << 8)).getRawPointer(),
      pgm_size_gs.PGM_SIZE() << 3);
   decaf_check(pgm_offset_gs.PGM_OFFSET() == 0);

   // Data cache shader comes from vertex shader register
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_offset_vs = getRegister<latte::SQ_PGM_CF_OFFSET_VS>(latte::Register::SQ_PGM_CF_OFFSET_VS);
   auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
   auto dcShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_vs.PGM_START() << 8)).getRawPointer(),
      pgm_size_vs.PGM_SIZE() << 3);
   decaf_check(pgm_offset_vs.PGM_OFFSET() == 0);

   // If Geometry shading is enabled, we need to have a geometry shader, and data-cache
   //  shaders must always be set if a geometry shader is used.
   decaf_check(!gsShaderBinary.empty());
   decaf_check(!dcShaderBinary.empty());

   // Need to generate the shader here...
   auto shaderDesc = spirv::GeometryShaderDesc { };
   shaderDesc.type = spirv::ShaderType::Geometry;
   shaderDesc.binary = gsShaderBinary;
   shaderDesc.dcBinary = dcShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::GS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texFormat[i] = spirvTextureTypeFromLatte(sq_tex_resource_word4.NUM_FORMAT_ALL());
   }

   shaderDesc.regs.sq_gs_vert_itemsize = getRegister<latte::SQ_GS_VERT_ITEMSIZE>(latte::Register::SQ_GS_VERT_ITEMSIZE);
   shaderDesc.regs.vgt_gs_out_prim_type = getRegister<latte::VGT_GS_OUT_PRIMITIVE_TYPE>(latte::Register::VGT_GS_OUT_PRIM_TYPE);
   shaderDesc.regs.vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   shaderDesc.regs.sq_gsvs_ring_itemsize = getRegister<uint32_t>(latte::Register::SQ_GSVS_RING_ITEMSIZE);
   shaderDesc.regs.pa_cl_vs_out_cntl = getRegister<latte::PA_CL_VS_OUT_CNTL>(latte::Register::PA_CL_VS_OUT_CNTL);

   for (auto i = 0; i < latte::MaxStreamOutBuffers; ++i) {
      // Note that these registers are not contiguous!
      shaderDesc.streamOutStride[i] = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + i * 16) << 2;
   }

   return shaderDesc;
}

spirv::PixelShaderDesc
Driver::getPixelShaderDesc()
{
   // Do not generate pixel shaders if rasterization is disabled
   auto pa_cl_clip_cntl = getRegister<latte::PA_CL_CLIP_CNTL>(latte::Register::PA_CL_CLIP_CNTL);
   if (pa_cl_clip_cntl.RASTERISER_DISABLE()) {
      return spirv::PixelShaderDesc();
   }

   decaf_check(mCurrentDraw->vertexShader);

   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
   auto pgm_offset_ps = getRegister<latte::SQ_PGM_CF_OFFSET_PS>(latte::Register::SQ_PGM_CF_OFFSET_PS);
   auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
   auto psShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_ps.PGM_START() << 8)).getRawPointer(),
      pgm_size_ps.PGM_SIZE() << 3);
   decaf_check(pgm_offset_ps.PGM_OFFSET() == 0);

   auto shaderDesc = spirv::PixelShaderDesc { };
   shaderDesc.type = spirv::ShaderType::Pixel;
   shaderDesc.binary = psShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
      shaderDesc.pixelOutType[i] = spirvPixelTypeFromLatte(cb_color_info.NUMBER_TYPE());
   }

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texFormat[i] = spirvTextureTypeFromLatte(sq_tex_resource_word4.NUM_FORMAT_ALL());
   }

   shaderDesc.regs.sq_pgm_resources_ps = getRegister<latte::SQ_PGM_RESOURCES_PS>(latte::Register::SQ_PGM_RESOURCES_PS);
   shaderDesc.regs.sq_pgm_exports_ps = getRegister<latte::SQ_PGM_EXPORTS_PS>(latte::Register::SQ_PGM_EXPORTS_PS);

   shaderDesc.regs.spi_ps_in_control_0 = getRegister<latte::SPI_PS_IN_CONTROL_0>(latte::Register::SPI_PS_IN_CONTROL_0);
   shaderDesc.regs.spi_ps_in_control_1 = getRegister<latte::SPI_PS_IN_CONTROL_1>(latte::Register::SPI_PS_IN_CONTROL_1);
   shaderDesc.regs.spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);

   shaderDesc.regs.cb_shader_control = getRegister<latte::CB_SHADER_CONTROL>(latte::Register::CB_SHADER_CONTROL);
   shaderDesc.regs.cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   shaderDesc.regs.db_shader_control = getRegister<latte::DB_SHADER_CONTROL>(latte::Register::DB_SHADER_CONTROL);

   for (auto i = 0; i < 32; ++i) {
      shaderDesc.regs.spi_ps_input_cntls[i] = getRegister<latte::SPI_PS_INPUT_CNTL_N>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
   }

   for (auto i = 0; i < 10; ++i) {
      shaderDesc.regs.spi_vs_out_ids[i] = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + i * 4);
   }

   return shaderDesc;
}

struct ShaderBinaryEntry
{
   ShaderBinaryEntry(std::string name, gsl::span<const uint8_t> binary) :
      name(name), binary(binary)
   {
   }

   std::string name;
   gsl::span<const uint8_t> binary;
};

using ShaderBinaries = std::vector<ShaderBinaryEntry>;

static void
dumpRawShaderBinaries(const ShaderBinaries &shaderBinaries,
                      std::string shaderName)
{
   // Write binary shaders to dump file
   for (auto shaderBinary : shaderBinaries) {
      auto filePathBinarySuffix = shaderBinary.name.empty() ?
         "" : fmt::format("_{}", shaderBinary.name);
      auto filePathBinary = fmt::format("dump/{}{}.bin", shaderName,
                                        filePathBinarySuffix);
      if (!platform::fileExists(filePathBinary)) {
         platform::createDirectory("dump");

         // Write Binary Output
         auto file = std::ofstream{ filePathBinary,
            std::ofstream::out | std::ofstream::binary };
         file.write(reinterpret_cast<const char *>(shaderBinary.binary.data()),
                    shaderBinary.binary.size());
      }
   }
}

static void
dumpRawShader(const spirv::ShaderDesc *desc,
              bool onlyDumpBinaries)
{
   auto shaderBinaries = ShaderBinaries { };
   auto shaderName = std::string { };

   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<const spirv::VertexShaderDesc*>(desc);
      auto vsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(vsDesc->binary.data()));
      auto fsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(vsDesc->fsBinary.data()));

      shaderName = fmt::format("vs_{:08x}_{:08x}", vsAddr, fsAddr);
      shaderBinaries.emplace_back("fs", vsDesc->fsBinary);
      shaderBinaries.emplace_back("", vsDesc->binary);
   } else if (desc->type == spirv::ShaderType::Geometry) {
      auto gsDesc = reinterpret_cast<const spirv::GeometryShaderDesc*>(desc);
      auto gsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(gsDesc->binary.data()));
      auto dcAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(gsDesc->dcBinary.data()));

      shaderName = fmt::format("gs_{:08x}_{:08x}", gsAddr, dcAddr);
      shaderBinaries.emplace_back("dc", gsDesc->dcBinary);
      shaderBinaries.emplace_back("", gsDesc->binary);
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<const spirv::PixelShaderDesc*>(desc);
      auto psAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(psDesc->binary.data()));

      shaderName = fmt::format("ps_{:08x}", psAddr);
      shaderBinaries.emplace_back("", psDesc->binary);
   } else {
      decaf_abort("Unexpected shader type");
   }

   // Dump binaries
   dumpRawShaderBinaries(shaderBinaries, shaderName);
   if (onlyDumpBinaries) {
      return;
   }

   // Dump disassembly
   auto outputStr = std::string { };
   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<const spirv::VertexShaderDesc*>(desc);
      auto fsDisasm = std::string { };
      auto vsDisasm = std::string { };

      if (!vsDesc->fsBinary.empty()) {
         fsDisasm = latte::disassemble(vsDesc->fsBinary, true);
      }

      if (!vsDesc->binary.empty()) {
         vsDisasm = latte::disassemble(vsDesc->binary, false);
      }

      outputStr += "Fetch Shader:\n";
      outputStr += fsDisasm + "\n\n";
      outputStr += "Vertex Shader:\n";
      outputStr += vsDisasm + "\n\n";
   } else if (desc->type == spirv::ShaderType::Geometry) {
      auto gsDesc = reinterpret_cast<const spirv::GeometryShaderDesc*>(desc);
      auto gsDisasm = std::string { };
      auto dcDisasm = std::string { };

      if (!gsDesc->dcBinary.empty()) {
         dcDisasm = latte::disassemble(gsDesc->dcBinary, false);
      }

      if (!gsDesc->binary.empty()) {
         gsDisasm = latte::disassemble(gsDesc->binary, false);
      }

      outputStr += "Geometry Shader:\n";
      outputStr += gsDisasm + "\n\n";
      outputStr += "DMA Copy Shader:\n";
      outputStr += dcDisasm + "\n\n";
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<const spirv::PixelShaderDesc*>(desc);
      auto psDisasm = std::string { };

      if (!psDesc->binary.empty()) {
         psDisasm = latte::disassemble(psDesc->binary, false);
      }

      outputStr += "Pixel Shader:\n";
      outputStr += psDisasm + "\n\n";
   } else {
      decaf_abort("Unexpected shader type");
   }

   // Write shader disassembly to dump file
   auto filePath = fmt::format("dump/{}.txt", shaderName);
   if (!platform::fileExists(filePath)) {
      platform::createDirectory("dump");

      // Write Text Output
      auto file = std::ofstream { filePath, std::ofstream::out };
      file << outputStr << std::endl;
   }
}

static void
dumpTranslatedShader(const spirv::ShaderDesc *desc,
                     const spirv::Shader *shader)
{
   auto shaderText = spirv::shaderToString(shader);
   auto shaderName = std::string { };
   auto outputStr = std::string { };

   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<const spirv::VertexShaderDesc*>(desc);
      auto vsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(vsDesc->binary.data()));
      auto fsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(vsDesc->fsBinary.data()));

      shaderName = fmt::format("vs_{:08x}_{:08x}", vsAddr, fsAddr);
   } else if (desc->type == spirv::ShaderType::Geometry) {
      auto gsDesc = reinterpret_cast<const spirv::GeometryShaderDesc*>(desc);
      auto gsAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(gsDesc->binary.data()));
      auto dcAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(gsDesc->dcBinary.data()));

      shaderName = fmt::format("gs_{:08x}_{:08x}", gsAddr, dcAddr);
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<const spirv::PixelShaderDesc*>(desc);
      auto psAddr = static_cast<uint32_t>(
         reinterpret_cast<uintptr_t>(psDesc->binary.data()));

      shaderName = fmt::format("ps_{:08x}", psAddr);
   } else {
      decaf_abort("Unexpected shader type");
   }

   if (desc->type == spirv::ShaderType::Vertex) {
      outputStr += "Compiled Vertex Shader:\n";
      outputStr += shaderText + "\n\n";
   } else if (desc->type == spirv::ShaderType::Geometry) {
      outputStr += "Compiled Geometry Shader:\n";
      outputStr += shaderText + "\n\n";
   } else if (desc->type == spirv::ShaderType::Pixel) {
      outputStr += "Compiled Pixel Shader:\n";
      outputStr += shaderText + "\n\n";
   } else {
      decaf_abort("Unexpected shader type");
   }

   // Write to dump file
   auto filePath = fmt::format("dump/{}.spv.txt", shaderName);
   if (!platform::fileExists(filePath)) {
      platform::createDirectory("dump");

      // Write Text Output
      auto file = std::ofstream { filePath, std::ofstream::out };
      file << outputStr << std::endl;

      // SPIRV Binary Output
      auto binFilePath = fmt::format("dump/{}.spv", shaderName);
      auto binFile = std::ofstream { binFilePath,
         std::ofstream::out | std::ofstream::binary };
      binFile.write(reinterpret_cast<const char*>(shader->binary.data()),
                    shader->binary.size() * sizeof(shader->binary[0]));
   }
}

bool
Driver::checkCurrentVertexShader()
{
   // We defer the hashing until after we check if this shader is even
   // actually enabled or not...  Performance !
   auto currentDescPrehash = getVertexShaderDesc();

   // Check if the shader stage is disabled
   if (currentDescPrehash.type == spirv::ShaderType::Unknown) {
      mCurrentDraw->vertexShader = nullptr;
      return true;
   }

   auto currentDesc =
      HashedDesc<spirv::VertexShaderDesc> { currentDescPrehash };

   if (mCurrentDraw->vertexShader &&
       mCurrentDraw->vertexShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mVertexShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentDraw->vertexShader = foundShader;
      return true;
   }

   foundShader = new VertexShaderObject();
   foundShader->desc = currentDesc;

   if (mDumpShaders) {
      dumpRawShader(&*currentDesc, mDumpShaderBinariesOnly);
   }

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate vertex shader");
   }

   if (mDumpShaders) {
      dumpTranslatedShader(&*currentDesc, &foundShader->shader);
   }

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4,
                                 foundShader->shader.binary.data()));
   foundShader->module = module;

   auto shaderAddr = static_cast<uint32_t>(
      reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("vs_{:08x}", shaderAddr).c_str());

   mCurrentDraw->vertexShader = foundShader;
   return true;
}

bool
Driver::checkCurrentGeometryShader()
{
   // We defer the hashing until after we check if this shader is even
   // actually enabled or not...  Performance !
   auto currentDescPrehash = getGeometryShaderDesc();

   // Check if the shader stage is disabled
   if (currentDescPrehash.type == spirv::ShaderType::Unknown) {
      mCurrentDraw->geometryShader = nullptr;
      return true;
   }

   auto currentDesc =
      HashedDesc<spirv::GeometryShaderDesc> { currentDescPrehash };

   if (mCurrentDraw->geometryShader &&
       mCurrentDraw->geometryShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mGeometryShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentDraw->geometryShader = foundShader;
      return true;
   }

   foundShader = new GeometryShaderObject();
   foundShader->desc = currentDesc;

   if (mDumpShaders) {
      dumpRawShader(&*currentDesc, mDumpShaderBinariesOnly);
   }

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate geometry shader");
   }

   if (mDumpShaders) {
      dumpTranslatedShader(&*currentDesc, &foundShader->shader);
   }

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4,
                                 foundShader->shader.binary.data()));
   foundShader->module = module;

   auto shaderAddr = static_cast<uint32_t>(
      reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("gs_{:08x}", shaderAddr).c_str());

   mCurrentDraw->geometryShader = foundShader;
   return true;
}

bool
Driver::checkCurrentPixelShader()
{
   // We defer the hashing until after we check if this shader is even
   // actually enabled or not...  Performance !
   auto currentDescPrehash = getPixelShaderDesc();

   // Check if the shader stage is disabled
   if (currentDescPrehash.type == spirv::ShaderType::Unknown) {
      mCurrentDraw->pixelShader = nullptr;
      return true;
   }

   auto currentDesc = HashedDesc<spirv::PixelShaderDesc> { currentDescPrehash };

   if (mCurrentDraw->pixelShader &&
       mCurrentDraw->pixelShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mPixelShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentDraw->pixelShader = foundShader;
      return true;
   }

   foundShader = new PixelShaderObject();
   foundShader->desc = currentDesc;

   if (mDumpShaders) {
      dumpRawShader(&*currentDesc, mDumpShaderBinariesOnly);
   }

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate pixel shader");
   }

   if (mDumpShaders) {
      dumpTranslatedShader(&*currentDesc, &foundShader->shader);
   }

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4,
                                 foundShader->shader.binary.data()));
   foundShader->module = module;

   auto shaderAddr = static_cast<uint32_t>(
      reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("ps_{:08x}", shaderAddr).c_str());

   mCurrentDraw->pixelShader = foundShader;
   return true;
}

bool
Driver::checkCurrentRectStubShader()
{
   if (!mCurrentDraw->isRectDraw) {
      mCurrentDraw->rectStubShader = nullptr;
      return true;
   }

   decaf_check(mCurrentDraw->vertexShader);

   auto currentDesc = HashedDesc<spirv::RectStubShaderDesc> {
      spirv::generateRectSubShaderDesc(&mCurrentDraw->vertexShader->shader) };

   if (mCurrentDraw->rectStubShader &&
       mCurrentDraw->rectStubShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mRectStubShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentDraw->rectStubShader = foundShader;
      return true;
   }

   foundShader = new RectStubShaderObject();
   foundShader->desc = currentDesc;

   if (!spirv::generateRectStub(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to generate rect stub shader");
   }

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4,
                                 foundShader->shader.binary.data()));
   foundShader->module = module;

   setVkObjectName(module,
                   fmt::format("rstub_{}", currentDesc->numVsExports).c_str());

   mCurrentDraw->rectStubShader = foundShader;
   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
