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
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texIsUint[i] = (sq_tex_resource_word4.NUM_FORMAT_ALL() == latte::SQ_NUM_FORMAT::INT);
   }

   shaderDesc.generateRectStub = mCurrentDrawDesc.isRectDraw;

   shaderDesc.regs.sq_pgm_resources_vs = getRegister<latte::SQ_PGM_RESOURCES_VS>(latte::Register::SQ_PGM_RESOURCES_VS);
   shaderDesc.regs.pa_cl_vs_out_cntl = getRegister<latte::PA_CL_VS_OUT_CNTL>(latte::Register::PA_CL_VS_OUT_CNTL);
   shaderDesc.regs.spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);

   for (auto i = 0u; i < 32; ++i) {
      shaderDesc.regs.sq_vtx_semantics[i] = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
   }

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
   // Do not generate geometry shaders if they are disabled
   auto vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   if (vgt_gs_mode.MODE() == latte::VGT_GS_ENABLE_MODE::OFF) {
      return spirv::GeometryShaderDesc();
   }

   decaf_check(mCurrentVertexShader);

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
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texIsUint[i] = (sq_tex_resource_word4.NUM_FORMAT_ALL() == latte::SQ_NUM_FORMAT::INT);
   }

   shaderDesc.regs.sq_gs_vert_itemsize = getRegister<latte::SQ_GS_VERT_ITEMSIZE>(latte::Register::SQ_GS_VERT_ITEMSIZE);
   shaderDesc.regs.vgt_gs_out_prim_type = getRegister<latte::VGT_GS_OUT_PRIMITIVE_TYPE>(latte::Register::VGT_GS_OUT_PRIM_TYPE);
   shaderDesc.regs.vgt_gs_mode = getRegister<latte::VGT_GS_MODE>(latte::Register::VGT_GS_MODE);
   shaderDesc.regs.sq_gsvs_ring_itemsize = getRegister<uint32_t>(latte::Register::SQ_GSVS_RING_ITEMSIZE);
   shaderDesc.regs.spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   shaderDesc.regs.pa_cl_vs_out_cntl = getRegister<latte::PA_CL_VS_OUT_CNTL>(latte::Register::PA_CL_VS_OUT_CNTL);

   for (auto i = 0; i < 10; ++i) {
      shaderDesc.regs.spi_vs_out_ids[i] = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + i * 4);
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
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_RESOURCE_WORD4_0 + 4 * resourceOffset);
      shaderDesc.texDims[i] = sq_tex_resource_word0.DIM();
      shaderDesc.texIsUint[i] = (sq_tex_resource_word4.NUM_FORMAT_ALL() == latte::SQ_NUM_FORMAT::INT);
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

   shaderDesc.regs.cb_shader_control = getRegister<latte::CB_SHADER_CONTROL>(latte::Register::CB_SHADER_CONTROL);
   shaderDesc.regs.cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   shaderDesc.regs.db_shader_control = getRegister<latte::DB_SHADER_CONTROL>(latte::Register::DB_SHADER_CONTROL);

   if (!mCurrentGeometryShader) {
      auto vsShader = reinterpret_cast<const spirv::VertexShader*>(&mCurrentVertexShader->shader);
      shaderDesc.vsOutputSemantics = vsShader->outputSemantics;
   } else {
      auto gsShader = reinterpret_cast<const spirv::GeometryShader*>(&mCurrentGeometryShader->shader);
      shaderDesc.vsOutputSemantics = gsShader->outputSemantics;
   }

   return shaderDesc;
}

static void dumpRawShader(spirv::ShaderDesc *desc)
{
   if (!gpu::config::dump_shaders) {
      return;
   }

   std::string shaderName;
   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<spirv::VertexShaderDesc*>(desc);
      auto vsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vsDesc->binary.data()));
      auto fsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vsDesc->fsBinary.data()));
      shaderName = fmt::format("vs_{:08x}_{:08x}", vsAddr, fsAddr);
   } else if (desc->type == spirv::ShaderType::Geometry) {
      auto gsDesc = reinterpret_cast<spirv::GeometryShaderDesc*>(desc);
      auto gsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gsDesc->binary.data()));
      auto dcAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gsDesc->dcBinary.data()));
      shaderName = fmt::format("gs_{:08x}_{:08x}", gsAddr, dcAddr);
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<spirv::PixelShaderDesc*>(desc);
      auto psAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psDesc->binary.data()));
      shaderName = fmt::format("ps_{:08x}", psAddr);
   } else {
      decaf_abort("Unexpected shader type");
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

   // Write to dump file
   auto filePath = fmt::format("dump/{}.txt", shaderName);
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

   std::string shaderName;
   if (desc->type == spirv::ShaderType::Vertex) {
      auto vsDesc = reinterpret_cast<spirv::VertexShaderDesc*>(desc);
      auto vsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vsDesc->binary.data()));
      auto fsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(vsDesc->fsBinary.data()));
      shaderName = fmt::format("vs_{:08x}_{:08x}", vsAddr, fsAddr);
   } else if (desc->type == spirv::ShaderType::Geometry) {
      auto gsDesc = reinterpret_cast<spirv::GeometryShaderDesc*>(desc);
      auto gsAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gsDesc->binary.data()));
      auto dcAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(gsDesc->dcBinary.data()));
      shaderName = fmt::format("gs_{:08x}_{:08x}", gsAddr, dcAddr);
   } else if (desc->type == spirv::ShaderType::Pixel) {
      auto psDesc = reinterpret_cast<spirv::PixelShaderDesc*>(desc);
      auto psAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psDesc->binary.data()));
      shaderName = fmt::format("ps_{:08x}", psAddr);
   } else {
      decaf_abort("Unexpected shader type");
   }

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

   // Write to dump file
   auto filePath = fmt::format("dump/{}.spv.txt", shaderName);
   if (!platform::fileExists(filePath)) {
      platform::createDirectory("dump");

      // Write Text Output
      auto file = std::ofstream { filePath, std::ofstream::out };
      file << outputStr << std::endl;

      // SPIRV Binary Output
      auto binFilePath = fmt::format("dump/{}.spv", shaderName);
      auto binFile = std::ofstream { binFilePath, std::ofstream::out | std::ofstream::binary };
      binFile.write(reinterpret_cast<const char*>(shader->binary.data()),
                    shader->binary.size() * sizeof(shader->binary[0]));

      // SPIRV Binary Output for Rect Stub
      if (desc->type == spirv::ShaderType::Vertex) {
         auto vsShader = reinterpret_cast<spirv::VertexShader*>(shader);
         if (!vsShader->rectStubBinary.empty()) {
            auto binFilePath = fmt::format("dump/{}.rects.spv", shaderName);
            auto binFile = std::ofstream { binFilePath, std::ofstream::out | std::ofstream::binary };
            binFile.write(reinterpret_cast<const char*>(vsShader->rectStubBinary.data()),
                          vsShader->rectStubBinary.size() * sizeof(vsShader->rectStubBinary[0]));
         }
      }
   }
}

bool
Driver::checkCurrentVertexShader()
{
   HashedDesc<spirv::VertexShaderDesc> currentDesc = getVertexShaderDesc();

   // Check if the shader stage is disabled
   if (currentDesc->type == spirv::ShaderType::Unknown) {
      mCurrentVertexShader = nullptr;
      return true;
   }

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

   auto shaderAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("vs_{:08x}", shaderAddr).c_str());

   if (!foundShader->shader.rectStubBinary.empty()) {
      auto rectStubModule = mDevice.createShaderModule(
         vk::ShaderModuleCreateInfo({}, foundShader->shader.rectStubBinary.size() * 4, foundShader->shader.rectStubBinary.data()));
      foundShader->rectStubModule = rectStubModule;
   }

   mCurrentVertexShader = foundShader;
   return true;
}

bool
Driver::checkCurrentGeometryShader()
{
   HashedDesc<spirv::GeometryShaderDesc> currentDesc = getGeometryShaderDesc();

   // Check if the shader stage is disabled
   if (currentDesc->type == spirv::ShaderType::Unknown) {
      mCurrentGeometryShader = nullptr;
      return true;
   }

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

   auto shaderAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("gs_{:08x}", shaderAddr).c_str());

   mCurrentGeometryShader = foundShader;
   return true;
}

bool Driver::checkCurrentPixelShader()
{
   HashedDesc<spirv::PixelShaderDesc> currentDesc = getPixelShaderDesc();

   // Check if the shader stage is disabled
   if (currentDesc->type == spirv::ShaderType::Unknown) {
      mCurrentPixelShader = nullptr;
      return true;
   }

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

   auto shaderAddr = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(currentDesc->binary.data()));
   setVkObjectName(module, fmt::format("ps_{:08x}", shaderAddr).c_str());

   mCurrentPixelShader = foundShader;
   return true;
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
