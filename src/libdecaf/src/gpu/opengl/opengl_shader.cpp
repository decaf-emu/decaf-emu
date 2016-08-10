#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/murmur3.h"
#include "common/platform_dir.h"
#include "common/strutils.h"
#include "decaf_config.h"
#include "glsl2_translate.h"
#include "gpu/latte_registers.h"
#include "gpu/microcode/latte_disassembler.h"
#include "opengl_constants.h"
#include "opengl_driver.h"
#include <fstream>
#include <glbinding/gl/gl.h>
#include <spdlog/spdlog.h>

namespace gpu
{

namespace opengl
{

// TODO: This is really some kind of 'nsight compat mode'...  Maybe
//  it should even be a configurable option (related to force_sync).
static const auto NO_PERSISTENT_MAP = false;

// This represents the number of bytes of padding to attach to the end
//  of an attribute buffer.  This is neccessary for cases where the game
//  fetches past the edge of a buffer, but does not use it.
static const auto BUFFER_PADDING = 16;

static void
dumpRawShader(const std::string &type, ppcaddr_t data, uint32_t size, bool isSubroutine = false)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   std::string filePath = fmt::format("dump/gpu_{}_{:08x}.txt", type, data);

   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   auto file = std::ofstream{ filePath, std::ofstream::out };

   // Disassemble
   auto output = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(data), size), isSubroutine);

   file << output << std::endl;
}

static void
dumpTranslatedShader(const std::string &type, ppcaddr_t data, const std::string &shaderSource)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   std::string filePath = fmt::format("dump/gpu_{}_{:08x}.glsl", type, data);

   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   auto file = std::ofstream{ filePath, std::ofstream::out };

   file << shaderSource << std::endl;
}

static std::string
getDataFormatName(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
      return "FMT_8";
   case latte::FMT_16:
      return "FMT_16";
   case latte::FMT_16_FLOAT:
      return "FMT_16_FLOAT";
   case latte::FMT_32:
      return "FMT_32";
   case latte::FMT_32_FLOAT:
      return "FMT_32_FLOAT";
   case latte::FMT_8_8:
      return "FMT_8_8";
   case latte::FMT_16_16:
      return "FMT_16_16";
   case latte::FMT_16_16_FLOAT:
      return "FMT_16_16_FLOAT";
   case latte::FMT_32_32:
      return "FMT_32_32";
   case latte::FMT_32_32_FLOAT:
      return "FMT_32_32_FLOAT";
   case latte::FMT_8_8_8:
      return "FMT_8_8_8";
   case latte::FMT_16_16_16:
      return "FMT_16_16_16";
   case latte::FMT_16_16_16_FLOAT:
      return "FMT_16_16_16_FLOAT";
   case latte::FMT_32_32_32:
      return "FMT_32_32_32";
   case latte::FMT_32_32_32_FLOAT:
      return "FMT_32_32_32_FLOAT";
   case latte::FMT_8_8_8_8:
      return "FMT_8_8_8_8";
   case latte::FMT_16_16_16_16:
      return "FMT_16_16_16_16";
   case latte::FMT_16_16_16_16_FLOAT:
      return "FMT_16_16_16_16_FLOAT";
   case latte::FMT_32_32_32_32:
      return "FMT_32_32_32_32";
   case latte::FMT_32_32_32_32_FLOAT:
      return "FMT_32_32_32_32_FLOAT";
   case latte::FMT_2_10_10_10:
      return "FMT_2_10_10_10";
   case latte::FMT_10_10_10_2:
      return "FMT_10_10_10_2";
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

static gl::GLint
getDataFormatComponents(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
      return 1;
   case latte::FMT_8_8:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
      return 2;
   case latte::FMT_8_8_8:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      return 3;
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_10_10_2:
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

static gl::GLint
getDataFormatComponentBits(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
   case latte::FMT_8_8:
   case latte::FMT_8_8_8:
   case latte::FMT_8_8_8_8:
      return 8;
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
      return 16;
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 32;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

static bool
getDataFormatIsFloat(latte::SQ_DATA_FORMAT format)
{
   switch (format) {

   case latte::FMT_16_FLOAT:
   case latte::FMT_32_FLOAT:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32_FLOAT:
      return true;
   case latte::FMT_8:
   case latte::FMT_16:
   case latte::FMT_32:
   case latte::FMT_8_8:
   case latte::FMT_16_16:
   case latte::FMT_32_32:
   case latte::FMT_8_8_8:
   case latte::FMT_16_16_16:
   case latte::FMT_32_32_32:
   case latte::FMT_2_10_10_10:
   case latte::FMT_10_10_10_2:
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_32_32_32_32:
      return false;
   default:
      decaf_abort(fmt::format("Unimplemented attribute format: {}", format));
   }
}

static gl::GLenum
getDataFormatGlType(latte::SQ_DATA_FORMAT format)
{
   // Some Special Cases
   switch (format) {
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
      return gl::GL_UNSIGNED_INT;
   }

   uint32_t bitCount = getDataFormatComponentBits(format);
   switch (bitCount) {
   case 8:
      return gl::GL_UNSIGNED_BYTE;
   case 16:
      return gl::GL_UNSIGNED_SHORT;
   case 32:
      return gl::GL_UNSIGNED_INT;
   default:
      decaf_abort(fmt::format("Unimplemented attribute bit count: {} for {}", bitCount, format));
   }
}

bool GLDriver::checkActiveShader()
{
   auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
   auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
   auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
   auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
   auto sx_alpha_ref = getRegister<latte::SX_ALPHA_REF>(latte::Register::SX_ALPHA_REF);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);

   if (!pgm_start_fs.PGM_START) {
      gLog->error("Fetch shader was not set");
      return false;
   }

   if (!pgm_start_vs.PGM_START) {
      gLog->error("Vertex shader was not set");
      return false;
   }

   if (!vgt_strmout_en.STREAMOUT() && !pgm_start_ps.PGM_START) {
      gLog->error("Pixel shader was not set (and transform feedback was not enabled)");
      return false;
   }

   auto fsPgmAddress = pgm_start_fs.PGM_START << 8;
   auto vsPgmAddress = pgm_start_vs.PGM_START << 8;
   auto psPgmAddress = pgm_start_ps.PGM_START << 8;
   auto fsPgmSize = pgm_size_fs.PGM_SIZE << 3;
   auto vsPgmSize = pgm_size_vs.PGM_SIZE << 3;
   auto psPgmSize = pgm_size_ps.PGM_SIZE << 3;
   auto alphaTestFunc = sx_alpha_test_control.ALPHA_FUNC();

   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE() || sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      alphaTestFunc = latte::REF_ALWAYS;
   }

   auto fsShaderKey = static_cast<uint64_t>(fsPgmAddress) << 32;
   auto vsShaderKey = static_cast<uint64_t>(vsPgmAddress) << 32;
   if (vgt_strmout_en.STREAMOUT()) {
      vsShaderKey ^= 1;
      for (auto i = 0u; i < latte::MaxStreamOutBuffers; ++i) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
         vsShaderKey ^= vgt_strmout_vtx_stride << (1 + 7 * i);
      }
   }
   auto psShaderKey = static_cast<uint64_t>(psPgmAddress) << 32;
   psShaderKey ^= static_cast<uint64_t>(alphaTestFunc) << 28;
   psShaderKey ^= cb_shader_mask.value & 0xFF;

   if (mActiveShader
    && mActiveShader->fetch && mActiveShader->fetchKey == fsShaderKey
    && mActiveShader->vertex && mActiveShader->vertexKey == vsShaderKey
    && mActiveShader->pixel && mActiveShader->pixelKey == psShaderKey) {
      // We already have the current shader bound, nothing special to do.
      return true;
   }

   auto shaderKey = ShaderKey { fsShaderKey, vsShaderKey, psShaderKey };
   auto &shader = mShaders[shaderKey];

   auto getProgramLog = [](auto program) {
      gl::GLint logLength = 0;
      std::string logMessage;
      gl::glGetProgramiv(program, gl::GL_INFO_LOG_LENGTH, &logLength);

      logMessage.resize(logLength);
      gl::glGetProgramInfoLog(program, logLength, &logLength, &logMessage[0]);
      return logMessage;
   };

   // Generate shader if needed
   if (!shader.object) {
      // Parse fetch shader if needed
      auto &fetchShader = mFetchShaders[fsShaderKey];

      if (!fetchShader.object) {
         auto aluDivisor0 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_0);
         auto aluDivisor1 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_1);

         fetchShader.cpuMemStart = fsPgmAddress;
         fetchShader.cpuMemEnd = fsPgmAddress + fsPgmSize;

         dumpRawShader("fetch", fsPgmAddress, fsPgmSize, true);
         fetchShader.disassembly = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(fsPgmAddress), fsPgmSize), true);

         if (!parseFetchShader(fetchShader, make_virtual_ptr<void>(fsPgmAddress), fsPgmSize)) {
            gLog->error("Failed to parse fetch shader");
            return false;
         }

         // Setup attrib format
         gl::glCreateVertexArrays(1, &fetchShader.object);

         auto bufferUsed = std::array<bool, latte::MaxAttributes> { false };
         auto bufferDivisor = std::array<uint32_t, latte::MaxAttributes> { 0 };

         for (auto &attrib : fetchShader.attribs) {
            auto resourceId = attrib.buffer + latte::SQ_VS_RESOURCE_BASE;
            if (resourceId >= latte::SQ_VS_ATTRIB_RESOURCE_0 && resourceId < latte::SQ_VS_ATTRIB_RESOURCE_0 + 0x10) {
               auto attribBufferId = resourceId - latte::SQ_VS_ATTRIB_RESOURCE_0;

               auto type = getDataFormatGlType(attrib.format);
               auto components = getDataFormatComponents(attrib.format);
               uint32_t divisor = 0;

               gl::glEnableVertexArrayAttrib(fetchShader.object, attrib.location);
               gl::glVertexArrayAttribIFormat(fetchShader.object, attrib.location, components, type, attrib.offset);
               gl::glVertexArrayAttribBinding(fetchShader.object, attrib.location, attribBufferId);

               if (attrib.type == latte::SQ_VTX_FETCH_TYPE::SQ_VTX_FETCH_INSTANCE_DATA) {
                  if (attrib.srcSelX == latte::SQ_SEL_W) {
                     divisor = 1;
                  } else if (attrib.srcSelX == latte::SQ_SEL_Y) {
                     divisor = aluDivisor0;
                  } else if (attrib.srcSelX == latte::SQ_SEL_Z) {
                     divisor = aluDivisor1;
                  } else {
                     decaf_abort(fmt::format("Unexpected SRC_SEL_X {} for alu divisor", attrib.srcSelX));
                  }
               }

               decaf_assert(!bufferUsed[attribBufferId] || bufferDivisor[attribBufferId] == divisor,
                  "Multiple attributes conflict on buffer divisor mode");

               bufferUsed[attribBufferId] = true;
               bufferDivisor[attribBufferId] = divisor;
            } else {
               decaf_abort("We do not yet support binding of non-attribute buffers");
            }
         }

         for (auto bufferId = 0; bufferId < latte::MaxAttributes; ++bufferId) {
            if (bufferUsed[bufferId]) {
               gl::glVertexArrayBindingDivisor(fetchShader.object, bufferId, bufferDivisor[bufferId]);
            }
         }
      }

      shader.fetch = &fetchShader;
      shader.fetchKey = fsShaderKey;

      // Compile vertex shader if needed
      auto &vertexShader = mVertexShaders[vsShaderKey];

      if (!vertexShader.object) {
         vertexShader.cpuMemStart = vsPgmAddress;
         vertexShader.cpuMemEnd = vsPgmAddress + vsPgmSize;

         dumpRawShader("vertex", vsPgmAddress, vsPgmSize);

         if (!compileVertexShader(vertexShader, fetchShader, make_virtual_ptr<uint8_t>(vsPgmAddress), vsPgmSize)) {
            gLog->error("Failed to recompile vertex shader");
            return false;
         }

         dumpTranslatedShader("vertex", vsPgmAddress, vertexShader.code);

         // Create OpenGL Shader
         const gl::GLchar *code[] = { vertexShader.code.c_str() };
         vertexShader.object = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, code);

         // Check if shader compiled & linked properly
         gl::GLint isLinked = 0;
         gl::glGetProgramiv(vertexShader.object, gl::GL_LINK_STATUS, &isLinked);

         if (!isLinked) {
            auto log = getProgramLog(vertexShader.object);
            gLog->error("OpenGL failed to compile vertex shader:\n{}", log);
            gLog->error("Fetch Disassembly:\n{}\n", fetchShader.disassembly);
            gLog->error("Shader Disassembly:\n{}\n", vertexShader.disassembly);
            gLog->error("Shader Code:\n{}\n", vertexShader.code);
            return false;
         }

         // Get uniform locations
         vertexShader.uniformRegisters = gl::glGetUniformLocation(vertexShader.object, "VR");

         // Get attribute locations
         vertexShader.attribLocations.fill(0);

         for (auto &attrib : fetchShader.attribs) {
            auto name = fmt::format("fs_out_{}", attrib.location);
            vertexShader.attribLocations[attrib.location] = gl::glGetAttribLocation(vertexShader.object, name.c_str());
         }
      }

      shader.vertex = &vertexShader;
      shader.vertexKey = vsShaderKey;

      if (vgt_strmout_en.STREAMOUT()) {

         // Transform feedback enabled; no pixel shader used
         // TODO: this should probably follow the rasterizer disable setting
         shader.pixel = nullptr;
         shader.pixelKey = 0;

      } else {

         // Transform feedback disabled; compile pixel shader if needed
         auto &pixelShader = mPixelShaders[psShaderKey];

         if (!pixelShader.object) {
            pixelShader.cpuMemStart = psPgmAddress;
            pixelShader.cpuMemEnd = psPgmAddress + psPgmSize;

            dumpRawShader("pixel", psPgmAddress, psPgmSize);

            if (!compilePixelShader(pixelShader, vertexShader, make_virtual_ptr<uint8_t>(psPgmAddress), psPgmSize)) {
               gLog->error("Failed to recompile pixel shader");
               return false;
            }

            dumpTranslatedShader("pixel", psPgmAddress, pixelShader.code);

            // Create OpenGL Shader
            const gl::GLchar *code[] = { pixelShader.code.c_str() };
            pixelShader.object = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, code);

            // Check if shader compiled & linked properly
            gl::GLint isLinked = 0;
            gl::glGetProgramiv(pixelShader.object, gl::GL_LINK_STATUS, &isLinked);

            if (!isLinked) {
               auto log = getProgramLog(pixelShader.object);
               gLog->error("OpenGL failed to compile pixel shader:\n{}", log);
               gLog->error("Shader Disassembly:\n{}\n", pixelShader.disassembly);
               gLog->error("Shader Code:\n{}\n", pixelShader.code);
               return false;
            }

            // Get uniform locations
            pixelShader.uniformRegisters = gl::glGetUniformLocation(pixelShader.object, "PR");
            pixelShader.uniformAlphaRef = gl::glGetUniformLocation(pixelShader.object, "uAlphaRef");
            pixelShader.sx_alpha_test_control = sx_alpha_test_control;
         }

         shader.pixel = &pixelShader;
         shader.pixelKey = psShaderKey;
      }

      // Create pipeline
      gl::glCreateProgramPipelines(1, &shader.object);
      gl::glUseProgramStages(shader.object, gl::GL_VERTEX_SHADER_BIT, shader.vertex->object);
      gl::glUseProgramStages(shader.object, gl::GL_FRAGMENT_SHADER_BIT, shader.pixel ? shader.pixel->object : 0);
   }

   // Set active shader
   mActiveShader = &shader;

   // Set alpha reference
   if (mActiveShader->pixel && alphaTestFunc != latte::REF_ALWAYS && alphaTestFunc != latte::REF_NEVER) {
      gl::glProgramUniform1f(mActiveShader->pixel->object, mActiveShader->pixel->uniformAlphaRef, sx_alpha_ref.ALPHA_REF);
   }

   // Bind fetch shader
   gl::glBindVertexArray(shader.fetch->object);

   // Bind vertex + pixel shader
   gl::glBindProgramPipeline(shader.object);
   return true;
}

bool GLDriver::checkActiveUniforms()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);

   if (!mActiveShader) {
      return true;
   }

   if (sq_config.DX9_CONSTS()) {
      // Upload uniform registers
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_256 / 4]);
         gl::glProgramUniform4fv(mActiveShader->vertex->object, mActiveShader->vertex->uniformRegisters, latte::MaxUniformRegisters, values);
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_0 / 4]);
         gl::glProgramUniform4fv(mActiveShader->pixel->object, mActiveShader->pixel->uniformRegisters, latte::MaxUniformRegisters, values);
      }
   } else {
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            auto sq_alu_const_cache_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + 4 * i);
            auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + 4 * i);
            auto used = mActiveShader->vertex->usedUniformBlocks[i];

            if (!used || !sq_alu_const_buffer_size_vs) {
               gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, i, 0);
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_alu_const_cache_vs << 8);
            auto size = sq_alu_const_buffer_size_vs << 8;

            // Check that we can fit the uniform block into OpenGL buffers
            decaf_assert(size <= gpu::opengl::MaxUniformBlockSize,
               fmt::format("Active uniform block with data size {} greater than what OpenGL supports {}", size, MaxUniformBlockSize));

            auto &buffer = mUniformBuffers[sq_alu_const_cache_vs];

            if (!buffer.object || buffer.allocatedSize < size) {
               if (buffer.object) {
                  gl::glDeleteBuffers(1, &buffer.object);
               }

               gl::glCreateBuffers(1, &buffer.object);
               gl::glNamedBufferStorage(buffer.object, size + BUFFER_PADDING, NULL, gl::GL_DYNAMIC_STORAGE_BIT);
               buffer.allocatedSize = size;
               buffer.dirtyAsBuffer = true;
            }

            if (buffer.dirtyAsBuffer) {
               // Calculate a new memory CRC
               uint64_t newHash[2] = { 0 };
               MurmurHash3_x64_128(block, size, 0, newHash);

               if (newHash[0] != buffer.cpuMemHash[0] || newHash[1] != buffer.cpuMemHash[1]) {
                  buffer.cpuMemHash[0] = newHash[0];
                  buffer.cpuMemHash[1] = newHash[1];
                  buffer.cpuMemStart = block.getAddress();
                  buffer.cpuMemEnd = buffer.cpuMemStart + size;

                  // Upload block
                  gl::glNamedBufferSubData(buffer.object, 0, size, block);
                  buffer.dirtyAsBuffer = false;
               }
            }

            // Bind block
            gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, i, buffer.object);
         }
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            auto sq_alu_const_cache_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_PS_0 + 4 * i);
            auto sq_alu_const_buffer_size_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + 4 * i);
            auto used = mActiveShader->pixel->usedUniformBlocks[i];

            if (!used || !sq_alu_const_buffer_size_ps) {
               gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, 16 + i, 0);
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_alu_const_cache_ps << 8);
            auto size = sq_alu_const_buffer_size_ps << 8;

            auto &buffer = mUniformBuffers[sq_alu_const_cache_ps];

            if (!buffer.object || buffer.allocatedSize < size) {
               if (buffer.object) {
                  gl::glDeleteBuffers(1, &buffer.object);
               }

               gl::glCreateBuffers(1, &buffer.object);
               gl::glNamedBufferStorage(buffer.object, size + BUFFER_PADDING, NULL, gl::GL_DYNAMIC_STORAGE_BIT);
               buffer.allocatedSize = size;
               buffer.dirtyAsBuffer = true;
            }

            if (buffer.dirtyAsBuffer) {
               // Calculate a new memory CRC
               uint64_t newHash[2] = { 0 };
               MurmurHash3_x64_128(block, size, 0, newHash);

               if (newHash[0] != buffer.cpuMemHash[0] || newHash[1] != buffer.cpuMemHash[1]) {
                  buffer.cpuMemHash[0] = newHash[0];
                  buffer.cpuMemHash[1] = newHash[1];
                  buffer.cpuMemStart = block.getAddress();
                  buffer.cpuMemEnd = buffer.cpuMemStart + size;

                  // Upload block
                  gl::glNamedBufferSubData(buffer.object, 0, size, block);
                  buffer.dirtyAsBuffer = false;
               }
            }

            // Bind block
            gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, 16 + i, buffer.object);
         }
      }
   }

   return true;
}

template<typename Type, int N>
static void
stridedMemcpy2(const void *srcBuffer,
               void *dstBuffer,
               size_t size,
               size_t offset,
               size_t stride,
               bool endian)
{
   static_assert(N > 0, "N must be greater than 0");
   static_assert(N <= 4, "N must be less or equal to 4");

   auto src = reinterpret_cast<const uint8_t *>(srcBuffer) + offset;
   auto end = reinterpret_cast<const uint8_t *>(srcBuffer) + size;
   auto dst = reinterpret_cast<uint8_t *>(dstBuffer) + offset;

   if (sizeof(Type) > 1 && endian) {
      while (src < end) {
         auto srcPtr = reinterpret_cast<const Type *>(src);
         auto dstPtr = reinterpret_cast<Type *>(dst);

         *dstPtr++ = byte_swap(*srcPtr++);
         if (N >= 2) {
            *dstPtr++ = byte_swap(*srcPtr++);
         }
         if (N >= 3) {
            *dstPtr++ = byte_swap(*srcPtr++);
         }
         if (N >= 4) {
            *dstPtr++ = byte_swap(*srcPtr++);
         }

         src += stride;
         dst += stride;
      }
   } else {
      while (src < end) {
         std::memcpy(dst, src, sizeof(Type) * N);
         src += stride;
         dst += stride;
      }
   }
}

static void
stridedMemcpy(const void *src,
              void *dst,
              size_t size,
              size_t offset,
              size_t stride,
              latte::SQ_ENDIAN endian,
              latte::SQ_DATA_FORMAT format)
{
   auto swap = (endian != latte::SQ_ENDIAN_NONE);

   switch (format) {
   case latte::FMT_8:
      stridedMemcpy2<uint8_t, 1>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_8_8:
      stridedMemcpy2<uint8_t, 2>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_8_8_8:
      stridedMemcpy2<uint8_t, 3>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_8_8_8_8:
      stridedMemcpy2<uint8_t, 4>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_16:
   case latte::FMT_16_FLOAT:
      stridedMemcpy2<uint16_t, 1>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_16_16:
   case latte::FMT_16_16_FLOAT:
      stridedMemcpy2<uint16_t, 2>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_FLOAT:
      stridedMemcpy2<uint16_t, 3>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
      stridedMemcpy2<uint16_t, 4>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_32:
   case latte::FMT_32_FLOAT:
      stridedMemcpy2<uint32_t, 1>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_32_32:
   case latte::FMT_32_32_FLOAT:
      stridedMemcpy2<uint32_t, 2>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_FLOAT:
      stridedMemcpy2<uint32_t, 3>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      stridedMemcpy2<uint32_t, 4>(src, dst, size, offset, stride, swap);
      break;
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
      stridedMemcpy2<uint32_t, 1>(src, dst, size, offset, stride, swap);
      break;
   default:
      decaf_abort(fmt::format("Unimplemented stride memcpy format {}", format));
   }
}

bool GLDriver::checkActiveAttribBuffers()
{
   if (!mActiveShader
    || !mActiveShader->fetch || !mActiveShader->fetch->attribs.size()
    || !mActiveShader->vertex || !mActiveShader->vertex->object) {
      return false;
   }

   for (auto i = 0u; i < latte::MaxAttributes; ++i) {
      auto resourceOffset = (latte::SQ_VS_ATTRIB_RESOURCE_0 + i) * 7;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_VTX_CONSTANT_WORD2_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_VTX_CONSTANT_WORD6_0 + 4 * resourceOffset);

      auto addr = sq_vtx_constant_word0.BASE_ADDRESS;
      auto size = sq_vtx_constant_word1.SIZE + 1;
      auto stride = sq_vtx_constant_word2.STRIDE();

      if (addr == 0 || size == 0) {
         continue;
      }

      auto &buffer = mAttribBuffers[addr];

      if (!buffer.object || buffer.allocatedSize < size) {
         if (buffer.object) {
            if (buffer.mappedBuffer) {
               gl::glUnmapNamedBuffer(buffer.object);
               buffer.mappedBuffer = nullptr;
            }

            gl::glDeleteBuffers(1, &buffer.object);
         }

         gl::glCreateBuffers(1, &buffer.object);
         gl::glNamedBufferStorage(buffer.object, size + BUFFER_PADDING, NULL, gl::GL_CLIENT_STORAGE_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
         buffer.allocatedSize = size;
      }

      auto ppcBuffer = mem::translate<const void>(addr);

      if (buffer.dirtyAsBuffer) {
         // Calculate a new memory CRC
         uint64_t newHash[2] = { 0 };
         MurmurHash3_x64_128(ppcBuffer, size, 0, newHash);

         if (newHash[0] != buffer.cpuMemHash[0] || newHash[1] != buffer.cpuMemHash[1]) {
            buffer.cpuMemHash[0] = newHash[0];
            buffer.cpuMemHash[1] = newHash[1];
            buffer.cpuMemStart = addr;
            buffer.cpuMemEnd = buffer.cpuMemStart + size;

            // Upload data
            if (!NO_PERSISTENT_MAP) {
               if (!buffer.mappedBuffer) {
                  buffer.mappedBuffer = gl::glMapNamedBufferRange(buffer.object, 0, size + BUFFER_PADDING, gl::GL_MAP_FLUSH_EXPLICIT_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
               }

               memcpy(buffer.mappedBuffer, ppcBuffer, size);
               gl::glFlushMappedNamedBufferRange(buffer.object, 0, size);
            } else {
               gl::glNamedBufferSubData(buffer.object, 0, size, ppcBuffer);
            }

            buffer.dirtyAsBuffer = false;
         }
      }

      gl::glBindVertexBuffer(i, buffer.object, 0, stride);
   }

   return true;
}

void GLDriver::createFeedbackBuffer(unsigned index)
{
   decaf_check(index <= 3);

   auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * index);
   auto vgt_strmout_buffer_size = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_SIZE_0 + 16 * index);

   auto addr = vgt_strmout_buffer_base << 8;
   auto size = vgt_strmout_buffer_size << 2;

   if (addr == 0 || size == 0) {
      gLog->error("Attempt to bind undefined feedback buffer {}", index);
      return;
   }

   auto &buffer = mFeedbackBuffers[addr];

   if (!buffer.object || buffer.size < size) {
      if (buffer.object) {
         gl::glDeleteBuffers(1, &buffer.object);
      }

      gl::glCreateBuffers(1, &buffer.object);
      // Explicitly hint feedback buffers to client storage, since we'll be
      //  reading from them via the CPU.
      gl::glNamedBufferStorage(buffer.object, size + BUFFER_PADDING, NULL, gl::GL_MAP_READ_BIT | gl::GL_CLIENT_STORAGE_BIT);
   }

   buffer.addr = addr;
   buffer.size = size;
}

bool GLDriver::checkActiveFeedbackBuffers()
{
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);
   auto vgt_strmout_buffer_en = getRegister<latte::VGT_STRMOUT_BUFFER_EN>(latte::Register::VGT_STRMOUT_BUFFER_EN);

   if (!vgt_strmout_en.STREAMOUT()) {
      return true;
   }

   for (auto index = 0u; index < latte::MaxStreamOutBuffers; ++index) {
      if (!(vgt_strmout_buffer_en.value & (1 << index))) {
         if (mFeedbackBufferCache[index].enable) {
            mFeedbackBufferCache[index].enable = false;
            gl::glBindBufferBase(gl::GL_TRANSFORM_FEEDBACK_BUFFER, index, 0);
         }
         continue;
      }

      mFeedbackBufferCache[index].enable = true;

      auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * index);
      auto addr = vgt_strmout_buffer_base << 8;
      auto &buffer = mFeedbackBuffers[addr];

      if (!buffer.object) {
         gLog->error("Attempt to use undefined feedback buffer {}", index);
         return false;
      }

      auto offset = mFeedbackBaseOffset[index];

      if (offset >= buffer.size) {
         gLog->error("Attempt to bind feedback buffer {} at offset {} >= size {}", index, offset, buffer.size);
         return false;
      }

      gl::glBindBufferRange(gl::GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer.object, offset, buffer.size - offset);
      mFeedbackCurrentOffset[index] = offset;
   }

   return true;
}

void GLDriver::copyFeedbackBuffer(unsigned index)
{
   auto vgt_strmout_buffer_base = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_BUFFER_BASE_0 + 16 * index);
   auto addr = vgt_strmout_buffer_base << 8;
   auto &buffer = mFeedbackBuffers[addr];
   decaf_check(buffer.object);

   auto copyOffset = mFeedbackBaseOffset[index];
   auto copySize = mFeedbackCurrentOffset[index] - copyOffset;

   if (copySize == 0) {
      return;
   }

   auto mappedBuffer = gl::glMapNamedBufferRange(buffer.object, copyOffset, copySize, gl::GL_MAP_READ_BIT);
   decaf_check(mappedBuffer);

   memcpy(mem::translate<char>(buffer.addr) + copyOffset,
          static_cast<char *>(mappedBuffer),
          copySize);

   // TODO: detect cases where a feedback buffer is also a vertex attribute
   //  buffer and skip the buffer upload in that case

   gl::glUnmapNamedBuffer(buffer.object);
}

bool GLDriver::parseFetchShader(FetchShader &shader, void *buffer, size_t size)
{
   auto program = reinterpret_cast<latte::ControlFlowInst *>(buffer);

   for (auto i = 0; i < size / 2; i++) {
      auto &cf = program[i];

      switch (cf.word1.CF_INST()) {
      case latte::SQ_CF_INST_VTX:
      case latte::SQ_CF_INST_VTX_TC:
      {
         auto vfPtr = reinterpret_cast<latte::VertexFetchInst *>(program + cf.word0.ADDR);
         auto count = ((cf.word1.COUNT_3() << 3) | cf.word1.COUNT()) + 1;

         for (auto j = 0u; j < count; ++j) {
            auto &vf = vfPtr[j];

            if (vf.word0.VTX_INST() != latte::SQ_VTX_INST_SEMANTIC) {
               gLog->error("Unexpected fetch shader VTX_INST {}", vf.word0.VTX_INST());
               continue;
            }

            // Parse new attrib
            shader.attribs.emplace_back();
            auto &attrib = shader.attribs.back();
            attrib.bytesPerElement = vf.word0.MEGA_FETCH_COUNT() + 1;
            attrib.format = vf.word1.DATA_FORMAT();
            attrib.buffer = vf.word0.BUFFER_ID();
            attrib.location = vf.gpr.DST_GPR();
            attrib.offset = vf.word2.OFFSET();
            attrib.formatComp = vf.word1.FORMAT_COMP_ALL();
            attrib.numFormat = vf.word1.NUM_FORMAT_ALL();
            attrib.endianSwap = vf.word2.ENDIAN_SWAP();
            attrib.dstSel[0] = vf.word1.DST_SEL_X();
            attrib.dstSel[1] = vf.word1.DST_SEL_Y();
            attrib.dstSel[2] = vf.word1.DST_SEL_Z();
            attrib.dstSel[3] = vf.word1.DST_SEL_W();
            attrib.type = vf.word0.FETCH_TYPE();
            attrib.srcSelX = vf.word0.SRC_SEL_X();
         }
         break;
      }
      case latte::SQ_CF_INST_RETURN:
      case latte::SQ_CF_INST_END_PROGRAM:
         return true;
      default:
         gLog->error("Unexpected fetch shader instruction {}", cf.word1.CF_INST());
      }

      if (cf.word1.END_OF_PROGRAM()) {
         return true;
      }
   }

   return false;
}

static const char *
getGLSLDataInFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT num, latte::SQ_FORMAT_COMP comp)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return "uint";
   }

   auto channels = getDataFormatComponents(format);

   switch (channels) {
   case 1:
      return "uint";
   case 2:
      return "uvec2";
   case 3:
      return "uvec3";
   case 4:
      return "uvec4";
   default:
      decaf_abort(fmt::format("Unimplemented attribute channel count: {} for {}", channels, format));
   }
}

bool GLDriver::compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   FetchShader::Attrib *semanticAttribs[32];
   std::memset(semanticAttribs, 0, sizeof(FetchShader::Attrib *) * 32);

   glsl2::Shader shader;
   shader.type = glsl2::Shader::VertexShader;

   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto resourceOffset = (latte::SQ_VS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   vertex.disassembly = latte::disassemble(gsl::as_span(buffer, size));

   if (!glsl2::translate(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode vertex shader\n{}", vertex.disassembly);
      return false;
   }

   vertex.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::MemoryWriter out;
   out << shader.fileHeader;

   out << "#define bswap16(v) (((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8))\n";
   out << "#define bswap32(v) (((v & 0xFF000000) >> 24) | ((v & 0x00FF0000) >> 8) | ((v & 0x0000FF00) << 8) | ((v & 0x000000FF) << 24))\n";
   out << "#define signext2(v) ((v ^ 0x2) - 0x2)\n";
   out << "#define signext8(v) ((v ^ 0x80) - 0x80)\n";
   out << "#define signext10(v) ((v ^ 0x200) - 0x200)\n";
   out << "#define signext16(v) ((v ^ 0x8000) - 0x8000)\n";

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      out << "//";
      out << " " << getDataFormatName(attrib.format);
      if (attrib.formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
         out << " SIGNED";
      } else {
         out << " UNSIGNED";
      }
      if (attrib.numFormat == latte::SQ_NUM_FORMAT_INT) {
         out << " INT";
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT_NORM) {
         out << " NORM";
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT_SCALED) {
         out << " SCALED";
      }
      if (attrib.endianSwap == latte::SQ_ENDIAN_NONE) {
         out << " SWAP_NONE";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_8IN32) {
         out << " SWAP_8IN32";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_8IN16) {
         out << " SWAP_8IN16";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_AUTO) {
         out << " SWAP_AUTO";
      }
      out << "\n";

      out << "layout(location = " << attrib.location << ")";
      out << " in "
          << getGLSLDataInFormat(attrib.format, attrib.numFormat, attrib.formatComp)
         << " fs_out_" << attrib.location << ";\n";
   }
   out << '\n';

   // Vertex Shader Exports
   decaf_check(!spi_vs_out_config.VS_PER_COMPONENT());
   vertex.outputMap.fill(0xff);

   for (auto i = 0u; i <= spi_vs_out_config.VS_EXPORT_COUNT(); i++) {
      auto regId = i / 4;
      auto spi_vs_out_id = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + 4 * regId);

      auto semanticNum = i % 4;
      uint8_t semanticId = 0xff;

      if (semanticNum == 0) {
         semanticId = spi_vs_out_id.SEMANTIC_0();
      } else if (semanticNum == 1) {
         semanticId = spi_vs_out_id.SEMANTIC_1();
      } else if (semanticNum == 2) {
         semanticId = spi_vs_out_id.SEMANTIC_2();
      } else if (semanticNum == 3) {
         semanticId = spi_vs_out_id.SEMANTIC_3();
      }

      if (semanticId != 0xff) {
         decaf_check(vertex.outputMap[semanticId] == 0xff);
         vertex.outputMap[semanticId] = i;

         out << "layout(location = " << i << ")";
         out << " out vec4 vs_out_" << semanticId << ";\n";
      }
   }
   out << '\n';

   // Transform feedback outputs
   for (auto buffer = 0u; buffer < latte::MaxStreamOutBuffers; ++buffer) {
      vertex.usedFeedbackBuffers[buffer] = !shader.feedbacks[buffer].empty();

      if (vertex.usedFeedbackBuffers[buffer]) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * buffer);
         auto stride = vgt_strmout_vtx_stride * 4;

         out
            << "layout(xfb_buffer = " << buffer
            << ", xfb_stride = " << stride
            << ") out block {\n";

         for (auto &xfb : shader.feedbacks[buffer]) {
            out << "   layout(xfb_offset = " << xfb.offset << ") out ";

            if (xfb.size == 1) {
               out << "float";
            } else {
               out << "vec" << xfb.size;
            }

            out << " feedback_" << xfb.streamIndex << "_" << xfb.offset << ";\n";
         }

         out << "};\n";
      }
   }
   out << '\n';

   out
      << "void main()\n"
      << "{\n"
      << shader.codeHeader;

   // Assign fetch shader output to our GPR
   for (auto i = 0u; i < 32; ++i) {
      auto sq_vtx_semantic = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
      auto id = sq_vtx_semantic.SEMANTIC_ID();

      if (id == 0xff) {
         continue;
      }

      auto attrib = semanticAttribs[id];

      if (!attrib) {
         gLog->error("Invalid semantic mapping: {}", id);
         continue;
      }


      fmt::MemoryWriter nameWriter;
      nameWriter << "fs_out_" << attrib->location;
      auto name = nameWriter.str();
      auto channels = getDataFormatComponents(attrib->format);
      auto isFloat = getDataFormatIsFloat(attrib->format);

      std::string chanVal[4];
      uint32_t chanBitCount[4];

      if (attrib->format == latte::FMT_10_10_10_2) {
         decaf_check(channels == 4);

         auto val = name;

         if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
            val = "bswap32(" + val + ")";
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
            decaf_abort("Unexpected 8IN16 swap for 10_10_10_2");
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_NONE) {
            // Nothing to do
         } else {
            decaf_abort("Unexpected endian swap mode");
         }

         chanVal[0] = std::string("((") + val + std::string(" >> 22) & 0x3ff)");
         chanVal[1] = std::string("((") + val + std::string(" >> 12) & 0x3ff)");
         chanVal[2] = std::string("((") + val + std::string(" >> 2) & 0x3ff)");
         chanVal[3] = std::string("((") + val + std::string(" >> 0) & 0x3)");

         if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
            chanVal[0] = "int(signext10(" + chanVal[0] + "))";
            chanVal[1] = "int(signext10(" + chanVal[1] + "))";
            chanVal[2] = "int(signext10(" + chanVal[2] + "))";
            chanVal[3] = "int(" + chanVal[3] + ")";
         } else {
            // Good to go!
         }

         chanBitCount[0] = 10;
         chanBitCount[1] = 10;
         chanBitCount[2] = 10;
         chanBitCount[3] = 2;
      } else if (attrib->format == latte::FMT_2_10_10_10) {
         decaf_check(channels == 4);

         auto val = name;

         if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
            val = "bswap32(" + val + ")";
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
            decaf_abort("Unexpected 8IN16 swap for 2_10_10_10");
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_NONE) {
            // Nothing to do
         } else {
            decaf_abort("Unexpected endian swap mode");
         }

         chanVal[0] = std::string("((") + val + std::string(" >> 30) & 0x3)");
         chanVal[1] = std::string("((") + val + std::string(" >> 20) & 0x3ff)");
         chanVal[2] = std::string("((") + val + std::string(" >> 10) & 0x3ff)");
         chanVal[3] = std::string("((") + val + std::string(" >> 0) & 0x3ff)");

         if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
            chanVal[0] + "int(" + chanVal[0] + ")";
            chanVal[1] + "int(signext10(" + chanVal[1] + "))";
            chanVal[2] + "int(signext10(" + chanVal[2] + "))";
            chanVal[3] + "int(signext10(" + chanVal[3] + "))";
         } else {
            // Good to go!
         }

         chanBitCount[0] = 2;
         chanBitCount[1] = 10;
         chanBitCount[2] = 10;
         chanBitCount[3] = 10;
      } else {
         auto compBits = getDataFormatComponentBits(attrib->format);

         for (auto i = 0; i < channels; ++i) {
            auto &val = chanVal[i];
            val = name;

            if (channels > 1) {
               if (i == 0) {
                  val += ".x";
               } else if (i == 1) {
                  val += ".y";
               } else if (i == 2) {
                  val += ".z";
               } else {
                  val += ".w";
               }
            }

            if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
               decaf_check(compBits == 32);
               val = "bswap32(" + val + ")";
            } else if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
               decaf_check(compBits == 16);
               val = "bswap16(" + val + ")";
            } else if (attrib->endianSwap == latte::SQ_ENDIAN_NONE) {
               // Nothing to do
            } else {
               decaf_abort("Unexpected endian swap mode");
            }

            if (isFloat) {
               if (compBits == 32) {
                  val = "uintBitsToFloat(" + val + ")";
               } else if (compBits == 16) {
                  val = "unpackHalf2x16(" + val + ").x";
               } else {
                  decaf_abort("Unexpected float component bit count");
               }
            } else {
               if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
                  if (compBits == 8) {
                     val = "int(signext8(" + val + "))";
                  } else if (compBits == 16) {
                     val = "int(signext16(" + val + "))";
                  } else if (compBits == 32) {
                     val = "int(" + val + ")";
                  } else {
                     decaf_abort("Unexpected signed component bit count");
                  }
               } else {
                  // Already the right format!
               }
            }

            chanBitCount[i] = compBits;
         }
      }

      for (auto i = 0; i < channels; ++i) {
         if (attrib->numFormat == latte::SQ_NUM_FORMAT_NORM) {
            uint32_t valMax = (1ul << chanBitCount[i]) - 1;

            if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
               chanVal[i] = fmt::format("clamp(float({}) / {}.0, -1.0, 1.0)", chanVal[i], valMax / 2);
            } else {
               chanVal[i] = fmt::format("float({}) / {}.0", chanVal[i], valMax);
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT_INT) {
            if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
               chanVal[i] = "intBitsToFloat(int(" + chanVal[i] + "))";
            } else {
               chanVal[i] = "uintBitsToFloat(uint(" + chanVal[i] + "))";
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT_SCALED) {
            chanVal[i] = "float(" + chanVal[i] + ")";
         } else {
            decaf_abort("Unexpected attribute number format");
         }
      }

      if (channels == 1) {
         out << "float _" << name << " = " << chanVal[0] << ";\n";
      } else if (channels == 2) {
         out << "vec2 _" << name << " = vec2(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ");\n";
      } else if (channels == 3) {
         out << "vec3 _" << name << " = vec3(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ",\n";
         out << "   " << chanVal[2] << ");\n";
      } else if (channels == 4) {
         out << "vec4 _" << name << " = vec4(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ",\n";
         out << "   " << chanVal[2] << ",\n";
         out << "   " << chanVal[3] << ");\n";
      } else {
         decaf_abort("Unexpected format channel count");
      }
      name = "_" + name;

      // Write the register assignment
      out << "R[" << (i + 1) << "] = ";

      switch (channels) {
      case 1:
         out << "vec4(" << name << ", 0.0, 0.0, 1.0);\n";
         break;
      case 2:
         out << "vec4(" << name << ", 0.0, 1.0);\n";
         break;
      case 3:
         out << "vec4(" << name << ", 1.0);\n";
         break;
      case 4:
         out << name << ";\n";
         break;
      }
   }

   out << '\n' << shader.codeBody << '\n';

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_POS:
         out << "gl_Position = exp_position_" << exp.id << ";\n";
         break;
      case latte::SQ_EXPORT_PARAM: {
         decaf_check(!spi_vs_out_config.VS_PER_COMPONENT());

         auto regId = exp.id / 4;
         auto spi_vs_out_id = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + 4 * regId);

         auto semanticNum = exp.id % 4;
         uint8_t semanticId = 0xff;

         if (semanticNum == 0) {
            semanticId = spi_vs_out_id.SEMANTIC_0();
         } else if (semanticNum == 1) {
            semanticId = spi_vs_out_id.SEMANTIC_1();
         } else if (semanticNum == 2) {
            semanticId = spi_vs_out_id.SEMANTIC_2();
         } else if (semanticNum == 3) {
            semanticId = spi_vs_out_id.SEMANTIC_3();
         }

         if (semanticId != 0xff) {
            out << "vs_out_" << semanticId << " = exp_param_" << exp.id << ";\n";
         } else {
            // This just helps when debugging to understand why it is missing...
            out << "// vs_out_none = exp_param_" << exp.id << ";\n";
         }
      } break;
      case latte::SQ_EXPORT_PIXEL:
         decaf_abort("Unexpected pixel export in vertex shader.");
      }
   }

   out << "}\n";
   out << "/* VERTEX SHADER DISASSEMBLY\n" << vertex.disassembly << "\n*/\n";
   out << "/* FETCH SHADER DISASSEMBLY\n" << fetch.disassembly << "\n*/\n";
   vertex.code = out.str();
   return true;
}

bool GLDriver::compilePixelShader(PixelShader &pixel, VertexShader &vertex, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_ps_in_control_0 = getRegister<latte::SPI_PS_IN_CONTROL_0>(latte::Register::SPI_PS_IN_CONTROL_0);
   auto spi_ps_in_control_1 = getRegister<latte::SPI_PS_IN_CONTROL_1>(latte::Register::SPI_PS_IN_CONTROL_1);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);

   glsl2::Shader shader;
   shader.type = glsl2::Shader::PixelShader;

   // Gather Samplers
   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto resourceOffset = (latte::SQ_PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   pixel.disassembly = latte::disassemble(gsl::as_span(buffer, size));

   if (!glsl2::translate(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode pixel shader\n{}", pixel.disassembly);
      return false;
   }

   pixel.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::MemoryWriter out;
   out << shader.fileHeader;
   out << "uniform float uAlphaRef;\n";

   if (spi_ps_in_control_0.POSITION_ENA()) {
      if (!spi_ps_in_control_0.POSITION_CENTROID()) {
         out << "layout(pixel_center_integer) ";
      }
      out << "in vec4 gl_FragCoord;\n";
   }

   // Pixel Shader Inputs
   std::array<bool, 256> semanticUsed = { false };
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      auto semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      decaf_check(vsOutputLoc != 0xff);

      if (semanticUsed[semanticId]) {
         continue;
      } else {
         semanticUsed[semanticId] = true;
      }

      out << "layout(location = " << vsOutputLoc << ")";

      if (spi_ps_input_cntl.FLAT_SHADE()) {
         out << " flat";
      }

      out << " in vec4 vs_out_" << semanticId << ";\n";
   }
   out << '\n';

   // Pixel Shader Exports
   auto maskBits = cb_shader_mask.value;

   for (auto i = 0; i < 8; ++i) {
      if (maskBits & 0xf) {
         out << "out vec4 ps_out_" << i << ";\n";
      }

      maskBits >>= 4;
   }
   out << '\n';

   out
      << "void main()\n"
      << "{\n"
      << shader.codeHeader;

   // Assign vertex shader output to our GPR
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      uint8_t semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      out << "R[" << i << "] = ";

      if (vsOutputLoc != 0xff) {
          out << "vs_out_" << semanticId;
      } else {
         if (spi_ps_input_cntl.DEFAULT_VAL() == 0) {
            out << "vec4(0, 0, 0, 0)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 1) {
            out << "vec4(0, 0, 0, 1)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 2) {
            out << "vec4(1, 1, 1, 0)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 3) {
            out << "vec4(1, 1, 1, 1)";
         } else {
            decaf_abort("Invalid PS input DEFAULT_VAL");
         }
      }

      out << ";\n";
   }

   if (spi_ps_in_control_0.POSITION_ENA()) {
      out << "R[" << spi_ps_in_control_0.POSITION_ADDR() << "] = gl_FragCoord;";
   }

   decaf_check(!spi_ps_in_control_0.PARAM_GEN());
   decaf_check(!spi_ps_in_control_1.GEN_INDEX_PIX());
   decaf_check(!spi_ps_in_control_1.FIXED_PT_POSITION_ENA());

   out << '\n' << shader.codeBody << '\n';

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_PIXEL:
      {
         auto mask = (cb_shader_mask.value >> (4 * exp.id)) & 0x0F;

         if (!mask) {
            gLog->warn("Export is masked by cb_shader_mask");
         } else {
            std::string strMask;

            if (mask & (1 << 0)) {
               strMask.push_back('x');
            }

            if (mask & (1 << 1)) {
               strMask.push_back('y');
            }

            if (mask & (1 << 2)) {
               strMask.push_back('z');
            }

            if (mask & (1 << 3)) {
               strMask.push_back('w');
            }

            if (sx_alpha_test_control.ALPHA_TEST_ENABLE() && !sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
               out << "// Alpha Test ";

               switch (sx_alpha_test_control.ALPHA_FUNC()) {
               case latte::REF_NEVER:
                  out << "REF_NEVER\n";
                  out << "discard;\n";
                  break;
               case latte::REF_LESS:
                  out << "REF_LESS\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w < uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_EQUAL:
                  out << "REF_EQUAL\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w == uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_LEQUAL:
                  out << "REF_LEQUAL\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w <= uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_GREATER:
                  out << "REF_GREATER\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w > uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_NOTEQUAL:
                  out << "REF_NOTEQUAL\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w != uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_GEQUAL:
                  out << "REF_GEQUAL\n";
                  out << "if (!(exp_pixel_" << exp.id << ".w >= uAlphaRef)) {\n";
                  out << "   discard;\n}\n";
                  break;
               case latte::REF_ALWAYS:
                  out << "REF_ALWAYS\n";
                  break;
               }
            }

            out
               << "ps_out_" << exp.id << "." << strMask
               << " = exp_pixel_" << exp.id << "." << strMask;

            out << ";\n";
         }
         break;
      }
      case latte::SQ_EXPORT_POS:
         decaf_abort("Unexpected position export in pixel shader.");
         break;
      case latte::SQ_EXPORT_PARAM:
         decaf_abort("Unexpected parameter export in pixel shader.");
         break;
      }
   }

   out << "}\n";

   out << "/* PIXEL SHADER DISASSEMBLY\n" << pixel.disassembly << "\n*/\n";

   pixel.code = out.str();
   return true;
}

} // namespace opengl

} // namespace gpu
