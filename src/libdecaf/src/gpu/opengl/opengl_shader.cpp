#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/platform_dir.h"
#include "common/strutils.h"
#include "decaf_config.h"
#include "glsl2_translate.h"
#include "gpu/latte_registers.h"
#include "gpu/microcode/latte_disassembler.h"
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
dumpShader(const std::string &type, ppcaddr_t data, uint32_t size, bool isSubroutine = false)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   std::string filePath = fmt::format("dump/gpu_{}_{:08x}.txt", type, data);

   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   // Write text of shader to shader_pixel_X.txt
   auto file = std::ofstream{ filePath, std::ofstream::out };

   // Disassemble
   auto output = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(data), size), isSubroutine);

   file
      << output << std::endl;
}

static gl::GLenum
getAttributeFormat(latte::SQ_DATA_FORMAT format, latte::SQ_FORMAT_COMP formatComp)
{
   bool isSigned = (formatComp == latte::SQ_FORMAT_COMP_SIGNED);

   switch (format) {
   case latte::FMT_8:
   case latte::FMT_8_8:
   case latte::FMT_8_8_8:
   case latte::FMT_8_8_8_8:
      return isSigned ? gl::GL_BYTE : gl::GL_UNSIGNED_BYTE;
   case latte::FMT_16:
   case latte::FMT_16_16:
   case latte::FMT_16_16_16:
   case latte::FMT_16_16_16_16:
      return isSigned ? gl::GL_SHORT : gl::GL_UNSIGNED_SHORT;
   case latte::FMT_16_FLOAT:
   case latte::FMT_16_16_FLOAT:
   case latte::FMT_16_16_16_FLOAT:
   case latte::FMT_16_16_16_16_FLOAT:
      return gl::GL_HALF_FLOAT;
   case latte::FMT_32:
   case latte::FMT_32_32:
   case latte::FMT_32_32_32:
   case latte::FMT_32_32_32_32:
      return isSigned ? gl::GL_INT : gl::GL_UNSIGNED_INT;
   case latte::FMT_32_FLOAT:
   case latte::FMT_32_32_FLOAT:
   case latte::FMT_32_32_32_FLOAT:
   case latte::FMT_32_32_32_32_FLOAT:
      return gl::GL_FLOAT;
   case latte::FMT_2_10_10_10:
      return gl::GL_UNSIGNED_INT_2_10_10_10_REV;
   default:
      gLog->error(fmt::format("Unsupported attribute format: {}", format));
      return gl::GL_BYTE;
   }
}

static gl::GLint
getAttributeComponents(latte::SQ_DATA_FORMAT format)
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
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   default:
      gLog->error(fmt::format("Unimplemented attribute format: {}", format));
      return 1;
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

   if (!pgm_start_fs.PGM_START) {
      gLog->error("Fetch shader was not set");
      return false;
   }

   if (!pgm_start_vs.PGM_START) {
      gLog->error("Vertex shader was not set");
      return false;
   }

   if (!pgm_start_ps.PGM_START) {
      gLog->error("Pixel shader was not set");
      return false;
   }

   auto fsPgmAddress = pgm_start_fs.PGM_START << 8;
   auto vsPgmAddress = pgm_start_vs.PGM_START << 8;
   auto psPgmAddress = pgm_start_ps.PGM_START << 8;
   auto fsPgmSize = pgm_size_fs.PGM_SIZE << 3;
   auto vsPgmSize = pgm_size_vs.PGM_SIZE << 3;
   auto psPgmSize = pgm_size_ps.PGM_SIZE << 3;
   auto alphaTestFunc = sx_alpha_test_control.ALPHA_FUNC().get();

   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE() || sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      alphaTestFunc = latte::REF_ALWAYS;
   }

   auto fsShaderKey = static_cast<uint64_t>(fsPgmAddress) << 32;
   auto vsShaderKey = static_cast<uint64_t>(vsPgmAddress) << 32;
   auto psShaderKey = static_cast<uint64_t>(psPgmAddress) << 32;
   psShaderKey ^= alphaTestFunc << 28;
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

   auto getProgramLog = [](auto program, auto getFn, auto getInfoFn) {
      gl::GLint logLength = 0;
      std::string logMessage;
      getFn(program, gl::GL_INFO_LOG_LENGTH, &logLength);

      logMessage.resize(logLength);
      getInfoFn(program, logLength, &logLength, &logMessage[0]);
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

         dumpShader("fetch", vsPgmAddress, vsPgmSize, true);
         fetchShader.disassembly = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(fsPgmAddress), fsPgmSize), true);

         if (!parseFetchShader(fetchShader, make_virtual_ptr<void>(fsPgmAddress), fsPgmSize)) {
            gLog->error("Failed to parse fetch shader");
            return false;
         }

         // Setup attrib format
         gl::glCreateVertexArrays(1, &fetchShader.object);

         std::array<bool, MAX_ATTRIB_COUNT> bufferUsed = { false };
         std::array<uint32_t, MAX_ATTRIB_COUNT> bufferDivisor = { 0 };

         for (auto &attrib : fetchShader.attribs) {
            auto resourceId = attrib.buffer + latte::SQ_VS_RESOURCE_BASE;
            if (resourceId >= latte::SQ_VS_ATTRIB_RESOURCE_0 && resourceId < latte::SQ_VS_ATTRIB_RESOURCE_0 + 0x10) {
               auto attribBufferId = resourceId - latte::SQ_VS_ATTRIB_RESOURCE_0;

               auto normalise = attrib.numFormat == latte::SQ_NUM_FORMAT_NORM ? gl::GL_TRUE : gl::GL_FALSE;
               auto type = getAttributeFormat(attrib.format, attrib.formatComp);
               auto components = getAttributeComponents(attrib.format);
               uint32_t divisor = 0;

               gl::glEnableVertexArrayAttrib(fetchShader.object, attrib.location);
               gl::glVertexArrayAttribFormat(fetchShader.object, attrib.location, components, type, normalise, attrib.offset);
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

         for (auto bufferId = 0; bufferId < MAX_ATTRIB_COUNT; ++bufferId) {
            if (bufferUsed[bufferId]) {
               gl::glVertexArrayBindingDivisor(fetchShader.object, bufferId, bufferDivisor[bufferId]);
            }
         }
      }

      // Compile vertex shader if needed
      auto &vertexShader = mVertexShaders[vsShaderKey];

      if (!vertexShader.object) {
         vertexShader.cpuMemStart = vsPgmAddress;
         vertexShader.cpuMemEnd = vsPgmAddress + vsPgmSize;

         dumpShader("vertex", vsPgmAddress, vsPgmSize);

         if (!compileVertexShader(vertexShader, fetchShader, make_virtual_ptr<uint8_t>(vsPgmAddress), vsPgmSize)) {
            gLog->error("Failed to recompile vertex shader");
            return false;
         }

         // Create OpenGL Shader
         const gl::GLchar *code[] = { vertexShader.code.c_str() };
         vertexShader.object = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, code);

         // Check if shader compiled & linked properly
         gl::GLint isLinked = 0;
         gl::glGetProgramiv(vertexShader.object, gl::GL_LINK_STATUS, &isLinked);

         if (!isLinked) {
            auto log = getProgramLog(vertexShader.object, gl::glGetProgramiv, gl::glGetProgramInfoLog);
            gLog->error("OpenGL failed to compile vertex shader:\n{}", log);
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

      // Compile pixel shader if needed
      auto &pixelShader = mPixelShaders[psShaderKey];

      if (!pixelShader.object) {
         pixelShader.cpuMemStart = psPgmAddress;
         pixelShader.cpuMemEnd = psPgmAddress + psPgmSize;

         dumpShader("pixel", psPgmAddress, psPgmSize);

         if (!compilePixelShader(pixelShader, vertexShader, make_virtual_ptr<uint8_t>(psPgmAddress), psPgmSize)) {
            gLog->error("Failed to recompile pixel shader");
            return false;
         }

         // Create OpenGL Shader
         const gl::GLchar *code[] = { pixelShader.code.c_str() };
         pixelShader.object = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, code);

         // Check if shader compiled & linked properly
         gl::GLint isLinked = 0;
         gl::glGetProgramiv(pixelShader.object, gl::GL_LINK_STATUS, &isLinked);

         if (!isLinked) {
            auto log = getProgramLog(pixelShader.object, gl::glGetProgramiv, gl::glGetProgramInfoLog);
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

      shader.fetch = &fetchShader;
      shader.vertex = &vertexShader;
      shader.pixel = &pixelShader;
      shader.fetchKey = fsShaderKey;
      shader.vertexKey = vsShaderKey;
      shader.pixelKey = psShaderKey;

      // Create pipeline
      gl::glGenProgramPipelines(1, &shader.object);
      gl::glUseProgramStages(shader.object, gl::GL_VERTEX_SHADER_BIT, shader.vertex->object);
      gl::glUseProgramStages(shader.object, gl::GL_FRAGMENT_SHADER_BIT, shader.pixel->object);
   }

   // Set active shader
   mActiveShader = &shader;

   // Set alpha reference
   if (alphaTestFunc != latte::REF_ALWAYS && alphaTestFunc != latte::REF_NEVER) {
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
         gl::glProgramUniform4fv(mActiveShader->vertex->object, mActiveShader->vertex->uniformRegisters, MAX_UNIFORM_REGISTERS, values);
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_0 / 4]);
         gl::glProgramUniform4fv(mActiveShader->pixel->object, mActiveShader->pixel->uniformRegisters, MAX_UNIFORM_REGISTERS, values);
      }
   } else {
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         for (auto i = 0u; i < MAX_UNIFORM_BLOCKS; ++i) {
            auto sq_alu_const_cache_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + 4 * i);
            auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + 4 * i);

            if (!sq_alu_const_buffer_size_vs) {
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_alu_const_cache_vs << 8);
            auto size = sq_alu_const_buffer_size_vs << 8;

            auto &ubo = mUniformBuffers[sq_alu_const_cache_vs];

            if (!ubo.object) {
               gl::glGenBuffers(1, &ubo.object);
            }

            // Upload block
            gl::glBindBuffer(gl::GL_UNIFORM_BUFFER, ubo.object);
            gl::glBufferData(gl::GL_UNIFORM_BUFFER, size, block, gl::GL_DYNAMIC_DRAW);

            // Bind block
            gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, i, ubo.object);
         }
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         for (auto i = 0u; i < MAX_UNIFORM_BLOCKS; ++i) {
            auto sq_alu_const_cache_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_PS_0 + 4 * i);
            auto sq_alu_const_buffer_size_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + 4 * i);

            if (!sq_alu_const_buffer_size_ps) {
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_alu_const_cache_ps << 8);
            auto size = sq_alu_const_buffer_size_ps << 8;

            auto &ubo = mUniformBuffers[sq_alu_const_cache_ps];

            if (!ubo.object) {
               gl::glGenBuffers(1, &ubo.object);
            }

            // Upload block
            gl::glBindBuffer(gl::GL_UNIFORM_BUFFER, ubo.object);
            gl::glBufferData(gl::GL_UNIFORM_BUFFER, size, block, gl::GL_DYNAMIC_DRAW);

            // Bind block
            gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, 16 + i, ubo.object);
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
   auto src = reinterpret_cast<const uint8_t *>(srcBuffer) + offset;
   auto end = reinterpret_cast<const uint8_t *>(srcBuffer) + size;
   auto dst = reinterpret_cast<uint8_t *>(dstBuffer) + offset;

   if (endian) {
      while (src < end) {
         auto srcPtr = reinterpret_cast<const Type *>(src);
         auto dstPtr = reinterpret_cast<Type *>(dst);

         for (auto i = 0u; i < N; ++i) {
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
   bool swap = (endian != latte::SQ_ENDIAN_NONE);

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
   std::array<AttributeBuffer *, 16> buffers;
   buffers.fill(nullptr);

   if (!mActiveShader
    || !mActiveShader->fetch || !mActiveShader->fetch->attribs.size()
    || !mActiveShader->vertex || !mActiveShader->vertex->object) {
      return false;
   }

   for (auto i = 0u; i < buffers.size(); ++i) {
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

      if (size % stride) {
         gLog->error("Error, size: {} is not multiple of stride: {}", size, stride);
         return false;
      }

      auto &buffer = mAttribBuffers[addr];

      if (!buffer.object || buffer.size < size) {
         if (buffer.object) {
            if (buffer.mappedBuffer) {
               gl::glUnmapNamedBuffer(buffer.object);
               buffer.mappedBuffer = nullptr;
            }

            gl::glDeleteBuffers(1, &buffer.object);
         }

         gl::glCreateBuffers(1, &buffer.object);
         gl::glNamedBufferStorage(buffer.object, size + BUFFER_PADDING, NULL, gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
      }

      if (!buffer.mappedBuffer) {
         if (!NO_PERSISTENT_MAP) {
            buffer.mappedBuffer = gl::glMapNamedBufferRange(buffer.object, 0, size + BUFFER_PADDING, gl::GL_MAP_FLUSH_EXPLICIT_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
         } else {
            buffer.mappedBuffer = gl::glMapNamedBufferRange(buffer.object, 0, size + BUFFER_PADDING, gl::GL_MAP_WRITE_BIT);
         }
      }

      buffer.addr = addr;
      buffer.size = size;
      buffer.stride = stride;
      buffers[i] = &buffer;
   }

   for (auto &attrib : mActiveShader->fetch->attribs) {
      auto resourceId = attrib.buffer + latte::SQ_VS_RESOURCE_BASE;
      if (resourceId >= latte::SQ_VS_ATTRIB_RESOURCE_0 && resourceId < latte::SQ_VS_ATTRIB_RESOURCE_0 + 0x10) {
         auto attribBufferId = resourceId - latte::SQ_VS_ATTRIB_RESOURCE_0;
         auto buffer = buffers[attribBufferId];

         if (!buffer || !buffer->object) {
            decaf_abort("Something odd happened with attribute buffers");
         }

         stridedMemcpy(mem::translate<const void>(buffer->addr),
            buffer->mappedBuffer,
            buffer->size,
            attrib.offset,
            buffer->stride,
            attrib.endianSwap,
            attrib.format);
      } else {
         decaf_abort("We do not yet support binding of non-attribute buffers");
      }
   }

   for (auto i = 0u; i < buffers.size(); i++) {
      auto buffer = buffers[i];

      if (buffer) {
         if (!NO_PERSISTENT_MAP) {
            gl::glFlushMappedNamedBufferRange(buffer->object, 0, buffer->size);
         } else {
            gl::glUnmapNamedBuffer(buffer->object);
            buffer->mappedBuffer = nullptr;
         }

         gl::glBindVertexBuffer(i, buffer->object, 0, buffer->stride);
      }
   }

   return true;
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

static size_t
getDataFormatChannels(latte::SQ_DATA_FORMAT format)
{
   switch (format) {
   case latte::FMT_8:
      return 1;
   case latte::FMT_4_4:
      return 2;
   case latte::FMT_3_3_2:
      return 3;
   case latte::FMT_16:
      return 1;
   case latte::FMT_16_FLOAT:
      return 1;
   case latte::FMT_8_8:
      return 2;
   case latte::FMT_5_6_5:
      return 3;
   case latte::FMT_6_5_5:
      return 3;
   case latte::FMT_1_5_5_5:
      return 4;
   case latte::FMT_4_4_4_4:
      return 4;
   case latte::FMT_5_5_5_1:
      return 4;
   case latte::FMT_32:
      return 1;
   case latte::FMT_32_FLOAT:
      return 1;
   case latte::FMT_16_16:
      return 2;
   case latte::FMT_16_16_FLOAT:
      return 2;
   case latte::FMT_8_24:
      return 2;
   case latte::FMT_8_24_FLOAT:
      return 2;
   case latte::FMT_24_8:
      return 2;
   case latte::FMT_24_8_FLOAT:
      return 2;
   case latte::FMT_10_11_11:
      return 3;
   case latte::FMT_10_11_11_FLOAT:
      return 3;
   case latte::FMT_11_11_10:
      return 3;
   case latte::FMT_11_11_10_FLOAT:
      return 3;
   case latte::FMT_2_10_10_10:
      return 4;
   case latte::FMT_8_8_8_8:
      return 4;
   case latte::FMT_10_10_10_2:
      return 4;
   case latte::FMT_X24_8_32_FLOAT:
      return 3;
   case latte::FMT_32_32:
      return 2;
   case latte::FMT_32_32_FLOAT:
      return 2;
   case latte::FMT_16_16_16_16:
      return 4;
   case latte::FMT_16_16_16_16_FLOAT:
      return 4;
   case latte::FMT_32_32_32_32:
      return 4;
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   case latte::FMT_1:
      return 1;
   case latte::FMT_GB_GR:
      return 2;
   case latte::FMT_BG_RG:
      return 2;
   case latte::FMT_32_AS_8:
      return 1;
   case latte::FMT_32_AS_8_8:
      return 1;
   case latte::FMT_5_9_9_9_SHAREDEXP:
      return 4;
   case latte::FMT_8_8_8:
      return 3;
   case latte::FMT_16_16_16:
      return 3;
   case latte::FMT_16_16_16_FLOAT:
      return 3;
   case latte::FMT_32_32_32:
      return 3;
   case latte::FMT_32_32_32_FLOAT:
      return 3;
   default:
      return 4;
   }
}

static const char *
getGLSLDataFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT num, latte::SQ_FORMAT_COMP comp)
{
   auto channels = getDataFormatChannels(format);

   if (num == latte::SQ_NUM_FORMAT_INT) {
      if (comp == latte::SQ_FORMAT_COMP_SIGNED) {
         switch (channels) {
         case 1:
            return "int";
         case 2:
            return "ivec2";
         case 3:
            return "ivec3";
         case 4:
            return "ivec4";
         }
      } else {
         switch (channels) {
         case 1:
            return "uint";
         case 2:
            return "uvec2";
         case 3:
            return "uvec3";
         case 4:
            return "uvec4";
         }
      }
   } else {
      switch (channels) {
      case 1:
         return "float";
      case 2:
         return "vec2";
      case 3:
         return "vec3";
      case 4:
         return "vec4";
      }
   }

   gLog->error("Unsupported GLSL data format, format={}, num={}, comp={}", format, num, comp);
   return "vec4";
}

bool GLDriver::compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   FetchShader::Attrib *semanticAttribs[32];
   std::memset(semanticAttribs, 0, sizeof(FetchShader::Attrib *) * 32);

   glsl2::Shader shader;
   shader.type = glsl2::Shader::VertexShader;

   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
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

   fmt::MemoryWriter out;
   out << shader.fileHeader;

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      out << "layout(location = " << attrib.location << ")";
      out << " in "
         << getGLSLDataFormat(attrib.format, attrib.numFormat, attrib.formatComp)
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

      out << "R[" << (i + 1) << "] = ";

      auto channels = getDataFormatChannels(attrib->format);

      switch (channels) {
      case 1:
         out << "vec4(fs_out_" << attrib->location << ", 0.0, 0.0, 1.0);\n";
         break;
      case 2:
         out << "vec4(fs_out_" << attrib->location << ", 0.0, 1.0);\n";
         break;
      case 3:
         out << "vec4(fs_out_" << attrib->location << ", 1.0);\n";
         break;
      case 4:
         out << "fs_out_" << attrib->location << ";\n";
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
   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
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
      gLog->error("Failed to decode vertex shader\n{}", pixel.disassembly);
      return false;
   }

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
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      auto semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      decaf_check(vsOutputLoc != 0xff);

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
         auto mask = cb_shader_mask.value >> (4 * exp.id);

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
