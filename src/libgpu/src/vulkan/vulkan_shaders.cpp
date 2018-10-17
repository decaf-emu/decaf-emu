#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "gpu_config.h"
#include "latte/latte_disassembler.h"

#include <common/log.h>
#include <common/platform_dir.h>
#include <fstream>

namespace vulkan
{

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

   spirv::VertexShaderDesc shaderDesc;

   shaderDesc.type = spirv::ShaderType::Vertex;
   shaderDesc.binary = vsShaderBinary;
   shaderDesc.fsBinary = fsShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
   }

   shaderDesc.regs.sq_pgm_resources_vs = getRegister<latte::SQ_PGM_RESOURCES_VS>(latte::Register::SQ_PGM_RESOURCES_VS);

   for (auto i = 0u; i < 32; ++i) {
      shaderDesc.regs.sq_vtx_semantics[i] = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
   }

   shaderDesc.regs.spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);

   for (auto i = 0; i < 10; ++i) {
      shaderDesc.regs.spi_vs_out_ids[i] = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + i * 4);
   }

   shaderDesc.instanceStepRates[0] = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_0);
   shaderDesc.instanceStepRates[1] = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_1);

   return shaderDesc;
}

spirv::GeometryShaderDesc
Driver::getGeometryShaderDesc()
{
   //decaf_abort("We do not currently support generation of geometry shaders.");
   return spirv::GeometryShaderDesc();

   gsl::span<uint8_t> gsShaderBinary;
   gsl::span<uint8_t> dcShaderBinary;

   // Geometry shader comes from geometry shader register
   auto pgm_start_gs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_GS);
   auto pgm_offset_gs = getRegister<latte::SQ_PGM_CF_OFFSET_GS>(latte::Register::SQ_PGM_CF_OFFSET_GS);
   auto pgm_size_gs = getRegister<latte::SQ_PGM_SIZE_GS>(latte::Register::SQ_PGM_SIZE_GS);
   gsShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_gs.PGM_START() << 8)).getRawPointer(),
      pgm_size_gs.PGM_SIZE() << 3);
   decaf_check(pgm_offset_gs.PGM_OFFSET() == 0);

   // Data cache shader comes from vertex shader register
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_offset_vs = getRegister<latte::SQ_PGM_CF_OFFSET_VS>(latte::Register::SQ_PGM_CF_OFFSET_VS);
   auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
   dcShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_vs.PGM_START() << 8)).getRawPointer(),
      pgm_size_vs.PGM_SIZE() << 3);
   decaf_check(pgm_offset_vs.PGM_OFFSET() == 0);

   // If Geometry shading is enabled, we need to have a geometry shader, and data-cache
   //  shaders must always be set if a geometry shader is used.
   decaf_check(!gsShaderBinary.empty());
   decaf_check(!dcShaderBinary.empty());

   // Need to generate the shader here...
   spirv::GeometryShaderDesc shaderDesc;

   shaderDesc.type = spirv::ShaderType::Geometry;
   shaderDesc.binary = gsShaderBinary;
   shaderDesc.dcBinary = dcShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::GS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
   }

   return shaderDesc;
}

spirv::PixelShaderDesc
Driver::getPixelShaderDesc()
{
   decaf_check(mCurrentVertexShader);

   gsl::span<uint8_t> psShaderBinary;

   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
   auto pgm_offset_ps = getRegister<latte::SQ_PGM_CF_OFFSET_PS>(latte::Register::SQ_PGM_CF_OFFSET_PS);
   auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
   psShaderBinary = gsl::make_span(
      phys_cast<uint8_t*>(phys_addr(pgm_start_ps.PGM_START() << 8)).getRawPointer(),
      pgm_size_ps.PGM_SIZE() << 3);
   decaf_check(pgm_offset_ps.PGM_OFFSET() == 0);

   spirv::PixelShaderDesc shaderDesc;

   shaderDesc.type = spirv::ShaderType::Pixel;
   shaderDesc.binary = psShaderBinary;

   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   shaderDesc.aluInstPreferVector = sq_config.ALU_INST_PREFER_VECTOR();

   for (auto i = 0; i < latte::MaxRenderTargets; ++i) {
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);

      spirv::ColorOutputType pixelOutType;
      switch (cb_color_info.NUMBER_TYPE()) {
      case latte::CB_NUMBER_TYPE::SINT:
         pixelOutType = spirv::ColorOutputType::SINT;
         break;
      case latte::CB_NUMBER_TYPE::UINT:
         pixelOutType = spirv::ColorOutputType::UINT;
         break;
      default:
         pixelOutType = spirv::ColorOutputType::FLOAT;
         break;
      }

      shaderDesc.pixelOutType[i] = pixelOutType;
   }

   for (auto i = 0; i < latte::MaxTextures; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
   }

   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
   shaderDesc.alphaRefFunc = sx_alpha_test_control.ALPHA_FUNC();
   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE() || sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      shaderDesc.alphaRefFunc = latte::REF_FUNC::ALWAYS;
   }

   shaderDesc.regs.sq_pgm_resources_ps = getRegister<latte::SQ_PGM_RESOURCES_PS>(latte::Register::SQ_PGM_RESOURCES_PS);
   shaderDesc.regs.sq_pgm_exports_ps = getRegister<latte::SQ_PGM_EXPORTS_PS>(latte::Register::SQ_PGM_EXPORTS_PS);

   shaderDesc.regs.spi_ps_in_control_0 = getRegister<latte::SPI_PS_IN_CONTROL_0>(latte::Register::SPI_PS_IN_CONTROL_0);
   shaderDesc.regs.spi_ps_in_control_1 = getRegister<latte::SPI_PS_IN_CONTROL_1>(latte::Register::SPI_PS_IN_CONTROL_1);

   for (auto i = 0; i < 32; ++i) {
      shaderDesc.regs.spi_ps_input_cntls[i] = getRegister<latte::SPI_PS_INPUT_CNTL_N>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
   }

   // TODO: In order to implement geometry shaders, we will have to use
   // those outputs instead, for now lets just check that there isn't one.
   decaf_check(!mCurrentGeometryShader);
   auto vsShader = reinterpret_cast<const spirv::VertexShader*>(&mCurrentVertexShader->shader);
   shaderDesc.vsOutputSemantics = vsShader->outputSemantics;

   return shaderDesc;
}

static void dumpRawShader(spirv::ShaderDesc *desc)
{
   if (!gpu::config::dump_shaders) {
      return;
   }

   std::string outputStr;
   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<spirv::VertexShaderDesc*>(desc);

      std::string fsDisasm, vsDisasm;
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
      auto gsDesc = reinterpret_cast<spirv::GeometryShaderDesc*>(desc);

      std::string gsDisasm, dcDisasm;
      if (!gsDesc->binary.empty()) {
         gsDisasm = latte::disassemble(gsDesc->binary, false);
      }
      if (!gsDesc->dcBinary.empty()) {
         dcDisasm = latte::disassemble(gsDesc->dcBinary, false);
      }

      outputStr += "Geometry Shader:\n";
      outputStr += gsDisasm + "\n\n";
      outputStr += "DMA Copy Shader:\n";
      outputStr += dcDisasm + "\n\n";
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<spirv::PixelShaderDesc*>(desc);

      std::string psDisasm;
      if (!psDesc->binary.empty()) {
         psDisasm = latte::disassemble(psDesc->binary, false);
      }

      outputStr += "Pixel Shader:\n";
      outputStr += psDisasm + "\n\n";
   } else {
      decaf_abort("Unexpected shader type");
   }

   // Print to log
   gLog->debug("{}", outputStr);

   // Write to dump file
   auto shaderType = static_cast<uint32_t>(desc->type);
   auto shaderAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(desc->binary.data()));
   auto filePath = fmt::format("dump/gpu_{}_{:x}_raw.txt", shaderType, shaderAddr);
   if (!platform::fileExists(filePath)) {
      platform::createDirectory("dump");

      // Write Text Output
      auto file = std::ofstream { filePath, std::ofstream::out };
      file << outputStr << std::endl;
   }
}

static void dumpTranslatedShader(spirv::ShaderDesc *desc, spirv::Shader *shader)
{
   if (!gpu::config::dump_shaders) {
      return;
   }

   auto shaderText = spirv::shaderToString(shader);

   std::string outputStr;
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

   // Print to log
   gLog->debug("{}", outputStr);

   // Write to dump file
   auto shaderType = static_cast<uint32_t>(desc->type);
   auto shaderAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(desc->binary.data()));
   auto filePath = fmt::format("dump/gpu_{}_{:x}_spv.txt", shaderType, shaderAddr);
   if (!platform::fileExists(filePath)) {
      platform::createDirectory("dump");

      // Write Text Output
      auto file = std::ofstream { filePath, std::ofstream::out };
      file << outputStr << std::endl;

      // SPIRV Binary Output
      auto binFilePath = fmt::format("dump/gpu_{}_{:x}_bin.spv", shaderType, shaderAddr);
      auto binFile = std::ofstream { binFilePath, std::ofstream::out | std::ofstream::binary };
      binFile.write(reinterpret_cast<const char*>(shader->binary.data()),
                    shader->binary.size() * sizeof(shader->binary[0]));
   }
}

bool
Driver::checkCurrentVertexShader()
{
   HashedDesc<spirv::VertexShaderDesc> currentDesc = getVertexShaderDesc();

   if (mCurrentVertexShader && mCurrentVertexShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mVertexShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentVertexShader = foundShader;
      return true;
   }

   foundShader = new VertexShaderObject();
   foundShader->desc = currentDesc;

   dumpRawShader(&*currentDesc);

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate vertex shader");
   }

   dumpTranslatedShader(&*currentDesc, &foundShader->shader);

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4, foundShader->shader.binary.data()));
   foundShader->module = module;

   mCurrentVertexShader = foundShader;
   return true;
}

bool
Driver::checkCurrentGeometryShader()
{
   decaf_check(mCurrentVertexShader);

   // Do not generate geometry shaders if they are disabled
   auto vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   if (vgt_gs_mode.MODE() == latte::VGT_GS_ENABLE_MODE::OFF) {
      mCurrentGeometryShader = nullptr;
      return true;
   }

   decaf_abort("We do not currently support generation of geometry shaders.");

   HashedDesc<spirv::GeometryShaderDesc> currentDesc = getGeometryShaderDesc();

   if (mCurrentGeometryShader && mCurrentGeometryShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mGeometryShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentGeometryShader = foundShader;
      return true;
   }

   foundShader = new GeometryShaderObject();
   foundShader->desc = currentDesc;

   dumpRawShader(&*currentDesc);

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate geometry shader");
   }

   dumpTranslatedShader(&*currentDesc, &foundShader->shader);

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4, foundShader->shader.binary.data()));
   foundShader->module = module;

   mCurrentGeometryShader = foundShader;
   return true;
}

bool Driver::checkCurrentPixelShader()
{
   decaf_check(mCurrentVertexShader);
   // technically we depend on this, but can't check since its possible that its just disabled
   //decaf_check(mCurrentGeometryShader);

   HashedDesc<spirv::PixelShaderDesc> currentDesc = getPixelShaderDesc();

   if (mCurrentPixelShader && mCurrentPixelShader->desc == currentDesc) {
      // Already active, nothing to do.
      return true;
   }

   auto& foundShader = mPixelShaders[currentDesc.hash()];
   if (foundShader) {
      mCurrentPixelShader = foundShader;
      return true;
   }

   foundShader = new PixelShaderObject();
   foundShader->desc = currentDesc;

   dumpRawShader(&*currentDesc);

   if (!spirv::translate(*currentDesc, &foundShader->shader)) {
      decaf_abort("Failed to translate pixel shader");
   }

   dumpTranslatedShader(&*currentDesc, &foundShader->shader);

   auto module = mDevice.createShaderModule(
      vk::ShaderModuleCreateInfo({}, foundShader->shader.binary.size() * 4, foundShader->shader.binary.data()));
   foundShader->module = module;

   mCurrentPixelShader = foundShader;
   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
