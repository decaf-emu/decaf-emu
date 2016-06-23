#include "common/log.h"
#include "common/strutils.h"
#include "glsl_generator.h"
#include "gpu/latte_registers.h"
#include "gpu/microcode/latte_decoder.h"
#include "opengl_driver.h"
#include <glbinding/gl/gl.h>
#include <spdlog/spdlog.h>

namespace gpu
{

namespace opengl
{

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

   if (mActiveShader
       && mActiveShader->fetch && mActiveShader->fetch->pgm_start_fs.PGM_START == pgm_start_fs.PGM_START
       && mActiveShader->vertex && mActiveShader->vertex->pgm_start_vs.PGM_START == pgm_start_vs.PGM_START
       && mActiveShader->pixel && mActiveShader->pixel->pgm_start_ps.PGM_START == pgm_start_ps.PGM_START) {
      // OpenGL shader matches latte shader
      return true;
   }

   if (!pgm_start_fs.PGM_START || !pgm_start_vs.PGM_START || !pgm_start_ps.PGM_START) {
      return false;
   }

   // Update OpenGL shader
   auto &fetchShader = mFetchShaders[pgm_start_fs.PGM_START];
   auto &vertexShader = mVertexShaders[pgm_start_vs.PGM_START];
   auto &pixelShader = mPixelShaders[pgm_start_ps.PGM_START];
   auto &shader = mShaders[ShaderKey { pgm_start_fs.PGM_START, pgm_start_vs.PGM_START, pgm_start_ps.PGM_START }];

   auto getProgramLog = [](auto program, auto getFn, auto getInfoFn) {
      gl::GLint logLength = 0;
      std::string logMessage;
      getFn(program, gl::GL_INFO_LOG_LENGTH, &logLength);

      logMessage.resize(logLength);
      getInfoFn(program, logLength, &logLength, &logMessage[0]);
      return logMessage;
   };

   // Genearte shader if needed
   if (!shader.object) {
      // Parse fetch shader if needed
      if (!fetchShader.object) {
         auto program = make_virtual_ptr<void>(pgm_start_fs.PGM_START << 8);
         auto size = pgm_size_fs.PGM_SIZE << 3;

         if (!parseFetchShader(fetchShader, program, size)) {
            gLog->error("Failed to parse fetch shader");
            return false;
         }

         // Setup attrib format
         gl::glCreateVertexArrays(1, &fetchShader.object);

         for (auto &attrib : fetchShader.attribs) {
            auto normalise = attrib.numFormat == latte::SQ_NUM_FORMAT_NORM ? gl::GL_TRUE : gl::GL_FALSE;
            auto type = getAttributeFormat(attrib.format, attrib.formatComp);
            auto components = getAttributeComponents(attrib.format);

            gl::glEnableVertexArrayAttrib(fetchShader.object, attrib.location);
            gl::glVertexArrayAttribFormat(fetchShader.object, attrib.location, components, type, normalise, attrib.offset);
            gl::glVertexArrayAttribBinding(fetchShader.object, attrib.location, attrib.buffer);
         }
      }

      // Compile vertex shader if needed
      if (!vertexShader.object) {
         auto program = make_virtual_ptr<uint8_t>(pgm_start_vs.PGM_START << 8);
         auto size = pgm_size_vs.PGM_SIZE << 3;

         if (!compileVertexShader(vertexShader, fetchShader, program, size)) {
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
      if (!pixelShader.object) {
         auto program = make_virtual_ptr<uint8_t>(pgm_start_ps.PGM_START << 8);
         auto size = pgm_size_ps.PGM_SIZE << 3;

         if (!compilePixelShader(pixelShader, program, size)) {
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
      }

      shader.fetch = &fetchShader;
      shader.vertex = &vertexShader;
      shader.pixel = &pixelShader;

      // Create pipeline
      gl::glGenProgramPipelines(1, &shader.object);
      gl::glUseProgramStages(shader.object, gl::GL_VERTEX_SHADER_BIT, shader.vertex->object);
      gl::glUseProgramStages(shader.object, gl::GL_FRAGMENT_SHADER_BIT, shader.pixel->object);
   }

   // Set active shader
   mActiveShader = &shader;

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
            auto resourceOffset = latte::SQ_VS_BUF_RESOURCE_0 + i * 7;
            auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_VTX_CONSTANT_WORD2_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word3 = getRegister<latte::SQ_VTX_CONSTANT_WORD3_N>(latte::Register::SQ_VTX_CONSTANT_WORD3_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_VTX_CONSTANT_WORD6_0 + 4 * resourceOffset);

            if (!sq_vtx_constant_word0.BASE_ADDRESS) {
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_vtx_constant_word0.BASE_ADDRESS);
            auto size = sq_vtx_constant_word1.SIZE + 1;
            auto values = size / 4;

            auto &ubo = mUniformBuffers[sq_vtx_constant_word0.BASE_ADDRESS];

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
            auto resourceOffset = latte::SQ_PS_BUF_RESOURCE_0 + i * 7;
            auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_VTX_CONSTANT_WORD2_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word3 = getRegister<latte::SQ_VTX_CONSTANT_WORD3_N>(latte::Register::SQ_VTX_CONSTANT_WORD3_0 + 4 * resourceOffset);
            auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_VTX_CONSTANT_WORD6_0 + 4 * resourceOffset);

            if (!sq_vtx_constant_word0.BASE_ADDRESS) {
               continue;
            }

            auto block = make_virtual_ptr<float>(sq_vtx_constant_word0.BASE_ADDRESS);
            auto size = sq_vtx_constant_word1.SIZE + 1;
            auto values = size / 4;

            auto &ubo = mUniformBuffers[sq_vtx_constant_word0.BASE_ADDRESS];

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
   default:
      gLog->error(fmt::format("Unimplemented stride memcpy format {}", format));
   }
}

bool GLDriver::checkActiveAttribBuffers()
{
   if (!mActiveShader
    || !mActiveShader->fetch || !mActiveShader->fetch->attribs.size()
    || !mActiveShader->vertex || !mActiveShader->vertex->object) {
      return false;
   }

   for (auto &attrib : mActiveShader->fetch->attribs) {
      auto index = attrib.buffer;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * (latte::SQ_VS_ATTRIB_RESOURCE_0 + index * 7));
      auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * (latte::SQ_VS_ATTRIB_RESOURCE_0 + index * 7));
      auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_VTX_CONSTANT_WORD2_0 + 4 * (latte::SQ_VS_ATTRIB_RESOURCE_0 + index * 7));
      auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_VTX_CONSTANT_WORD6_0 + 4 * (latte::SQ_VS_ATTRIB_RESOURCE_0 + index * 7));

      if (sq_vtx_constant_word6.TYPE() != latte::SQ_TEX_VTX_VALID_BUFFER) {
         gLog->error("No valid buffer set for attrib resource {}", index);
         return false;
      }

      auto addr = sq_vtx_constant_word0.BASE_ADDRESS;
      auto size = sq_vtx_constant_word1.SIZE + 1;
      auto stride = sq_vtx_constant_word2.STRIDE();
      auto &buffer = mAttribBuffers[addr];

      if (size % stride) {
         gLog->error("Error, size: {} is not multiple of stride: {}", size, stride);
         return false;
      }

      if (!buffer.object || buffer.size < size) {
         if (buffer.object) {
            gl::glUnmapNamedBuffer(buffer.object);
            gl::glDeleteBuffers(1, &buffer.object);
         }

         gl::glCreateBuffers(1, &buffer.object);
         gl::glNamedBufferStorage(buffer.object, size, NULL, gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
         buffer.mappedBuffer = gl::glMapNamedBufferRange(buffer.object, 0, size, gl::GL_MAP_FLUSH_EXPLICIT_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
         buffer.size = size;
      }

      stridedMemcpy(mem::translate<const void>(addr),
                    buffer.mappedBuffer,
                    buffer.size,
                    attrib.offset,
                    stride,
                    attrib.endianSwap,
                    attrib.format);

      gl::glFlushMappedNamedBufferRange(buffer.object, 0, buffer.size);
      gl::glBindVertexBuffer(attrib.buffer, buffer.object, 0, stride);
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
         auto count = (cf.word1.COUNT_3() << 3) | cf.word1.COUNT();

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

static void
writeHeader(fmt::MemoryWriter &out)
{
   out << "#version 420 core\n";
}

static void
writeUniforms(fmt::MemoryWriter &out, latte::Shader &shader, latte::SQ_CONFIG sq_config, std::array<SamplerType, MAX_SAMPLERS_PER_TYPE> &samplerTypes)
{
   if (sq_config.DX9_CONSTS()) {
      // Uniform registers
      out << "uniform vec4 ";

      if (shader.type == latte::Shader::Vertex) {
         out << "VR";
      } else {
         out << "PR";
      }

      out << "[" << MAX_UNIFORM_REGISTERS << "];\n";
   } else {
      // Uniform blocks
      for (uint32_t id = 0; id < MAX_UNIFORM_BLOCKS; ++id) {
         if (shader.type == latte::Shader::Vertex) {
            out << "layout (binding = " << id << ") uniform ";
            out << "VertexBlock_" << id;
         } else {
            out << "layout (binding = " << (16+id) << ") uniform ";
            out << "PixelBlock_" << id;
         }

         out
            << " {\n"
            << "   vec4 values[];\n"
            << "} ";

         if (shader.type == latte::Shader::Vertex) {
            out << "VB_";
         } else {
            out << "PB_";
         }

         out << id << ";\n";
      }
   }

   // Samplers
   for (auto id : shader.samplersUsed) {
      auto type = samplerTypes[id];

      out << "layout (binding = " << id << ") uniform ";

      switch (type) {
      case SamplerType::Sampler2D:
         out << "sampler2D";
         break;
      case SamplerType::Sampler2DArray:
         out << "sampler2DArray";
         break;
      default:
         gLog->error("Unsupported sampler type: {}", static_cast<uint32_t>(type));
      }

      out << " sampler_" << id << ";\n";
   }

   // Redeclare gl_PerVertex for Vertex Shaders
   if (shader.type == latte::Shader::Vertex) {
      out
         << "out gl_PerVertex {\n"
         << "   vec4 gl_Position;\n"
         << "};";
   }
}

static void
writeLocals(fmt::MemoryWriter &out, latte::Shader &shader)
{
   // Registers
   auto largestGpr = -1;
   auto largestTmp = -1;

   for (auto id : shader.gprsUsed) {
      if (id >= latte::SQ_ALU_TMP_REGISTER_FIRST) {
         largestTmp = std::max(largestTmp, static_cast<int>(latte::SQ_ALU_TMP_REGISTER_LAST - id));
      } else {
         largestGpr = std::max(largestGpr, static_cast<int>(id));
      }
   }

   if (largestGpr > -1) {
      out << "vec4 R[" << (largestGpr + 1) << "];\n";
   }

   if (largestTmp > -1) {
      out << "vec4 T[" << (largestTmp + 1) << "];\n";
   }

   // Previous Scalar
   if (shader.psUsed.size()) {
      out << "float PS;\n"
          << "float PSo;\n";
   }

   // Previous Vector
   if (shader.pvUsed.size()) {
      out << "vec4 PV;\n"
          << "vec4 PVo = vec4(0, 0, 0, 0);\n";
   }

   // Address Register (TODO: Only print if used)
   out << "ivec4 AR;\n";

   // Loop Index (TODO: Only print if used)
   out << "int AL;\n";
}

static void
writeExports(fmt::MemoryWriter &out, latte::Shader &shader)
{
   for (auto exp : shader.exports) {
      switch (exp->exportType) {
      case latte::SQ_EXPORT_POS:
         out << "vec4 exp_position_" << (exp->arrayBase - 60) << ";\n";
         break;
      case latte::SQ_EXPORT_PARAM:
         out << "vec4 exp_param_" << exp->arrayBase << ";\n";
         break;
      case latte::SQ_EXPORT_PIXEL:
         out << "vec4 exp_pixel_" << exp->arrayBase << ";\n";
         break;
      }
   }
}

static SamplerType
getSamplerType(latte::SQ_TEX_DIM dim, latte::SQ_NUM_FORMAT numFormat, latte::SQ_FORMAT_COMP formatComp)
{
   // TODO: Use numFormat and formatComp for signed/unsigned sampler
   switch (dim) {
   case latte::SQ_TEX_DIM_1D:
      return SamplerType::Sampler1D;
   case latte::SQ_TEX_DIM_2D:
      return SamplerType::Sampler2D;
   case latte::SQ_TEX_DIM_2D_ARRAY:
      return SamplerType::Sampler2DArray;
   default:
      gLog->error(fmt::format("Unimplemented sampler type {}", dim));
      return SamplerType::Invalid;
   }
}

bool GLDriver::compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   fmt::MemoryWriter out;
   latte::Shader shader;
   std::string body;
   FetchShader::Attrib *semanticAttribs[32];
   memset(semanticAttribs, 0, sizeof(FetchShader::Attrib *) * 32);

   shader.type = latte::Shader::Vertex;

   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * (latte::SQ_VS_TEX_RESOURCE_0 + i * 7));
      shader.samplers.emplace_back(sq_tex_resource_word0.DIM());
   }

   if (!latte::decode(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode vertex shader");
      return false;
   }

   latte::disassemble(shader, vertex.disassembly);

   if (!latte::blockify(shader)) {
      gLog->error("Failed to blockify vertex shader");
      return false;
   }

   if (!glsl::generateBody(shader, body)) {
      gLog->warn("Failed to translate 100% of instructions for vertex shader");
   }

   // Get vertex sampler types
   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * (latte::SQ_VS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * (latte::SQ_VS_TEX_RESOURCE_0 + i * 7));

      vertex.samplerTypes[i] = getSamplerType(sq_tex_resource_word0.DIM(),
                                              sq_tex_resource_word4.NUM_FORMAT_ALL(),
                                              sq_tex_resource_word4.FORMAT_COMP_X());
   }

   // Write header
   writeHeader(out);

   // Uniforms
   writeUniforms(out, shader, sq_config, vertex.samplerTypes);
   out << '\n';

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      out << "in "
          << getGLSLDataFormat(attrib.format, attrib.numFormat, attrib.formatComp)
          << " fs_out_" << attrib.location << ";\n";
   }
   out << '\n';

   // Vertex Shader Exports
   for (auto i = 0u; i <= spi_vs_out_config.VS_EXPORT_COUNT(); i++) {
      out << "out vec4 vs_out_" << i << ";\n";
   }
   out << '\n';

   // Program code
   out << "void main()\n"
       << "{\n";

   writeLocals(out, shader);
   writeExports(out, shader);

   // Initialise registers
   if (shader.gprsUsed.find(0) != shader.gprsUsed.end()) {
      // TODO: Check which order of VertexID and InstanceID for r0.x, r0.y
      out << "R[0] = vec4(intBitsToFloat(gl_VertexID), intBitsToFloat(gl_InstanceID), 0.0, 0.0);\n";
   }

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

   out << '\n' << body << '\n';

   for (auto exp : shader.exports) {
      switch (exp->exportType) {
      case latte::SQ_EXPORT_POS:
         out << "gl_Position = exp_position_" << (exp->arrayBase - 60) << ";\n";
         break;
      case latte::SQ_EXPORT_PARAM:
         // TODO: Use vs_out semantics?
         out << "vs_out_" << exp->arrayBase << " = exp_param_" << exp->arrayBase << ";\n";
         break;
      case latte::SQ_EXPORT_PIXEL:
         throw std::logic_error("Unexpected pixel export in vertex shader.");
         break;
      }
   }

   out << "}\n";

   out << "/*\n" << vertex.disassembly << "\n*/\n";

   vertex.code = out.str();
   return true;
}

bool GLDriver::compilePixelShader(PixelShader &pixel, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_ps_in_control_0 = getRegister<latte::SPI_PS_IN_CONTROL_0>(latte::Register::SPI_PS_IN_CONTROL_0);
   auto spi_ps_in_control_1 = getRegister<latte::SPI_PS_IN_CONTROL_1>(latte::Register::SPI_PS_IN_CONTROL_1);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   fmt::MemoryWriter out;
   latte::Shader shader;
   std::string body;

   shader.type = latte::Shader::Pixel;

   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      shader.samplers.emplace_back(sq_tex_resource_word0.DIM());
   }

   if (!latte::decode(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode pixel shader");
      return false;
   }

   latte::disassemble(shader, pixel.disassembly);

   if (!latte::blockify(shader)) {
      gLog->error("Failed to blockify pixel shader");
      return false;
   }

   if (!glsl::generateBody(shader, body)) {
      gLog->warn("Failed to translate 100% of instructions for pixel shader");
   }

   // Get pixel sampler types
   for (auto i = 0; i < MAX_SAMPLERS_PER_TYPE; ++i) {
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));
      auto sq_tex_resource_word4 = getRegister<latte::SQ_TEX_RESOURCE_WORD4_N>(latte::Register::SQ_TEX_RESOURCE_WORD4_0 + 4 * (latte::SQ_PS_TEX_RESOURCE_0 + i * 7));

      pixel.samplerTypes[i] = getSamplerType(sq_tex_resource_word0.DIM(),
                                             sq_tex_resource_word4.NUM_FORMAT_ALL(),
                                             sq_tex_resource_word4.FORMAT_COMP_X());
   }

   // Write header
   writeHeader(out);

   // Uniforms
   writeUniforms(out, shader, sq_config, pixel.samplerTypes);
   out << '\n';

   // Pixel Shader Inputs
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);

      if (spi_ps_input_cntl.FLAT_SHADE()) {
         out << "flat ";
      }

      out << "in vec4 vs_out_" << spi_ps_input_cntl.SEMANTIC() << ";\n";
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

   // Program code
   out << "void main()\n"
      << "{\n";

   writeLocals(out, shader);
   writeExports(out, shader);

   // Assign vertex shader output to our GPR
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      auto id = spi_ps_input_cntl.SEMANTIC();

      if (id == 0xff) {
         continue;
      }

      out << "R[" << i << "] = vs_out_" << id << ";\n";
   }

   out << '\n' << body << '\n';

   for (auto exp : shader.exports) {
      switch (exp->exportType) {
      case latte::SQ_EXPORT_PIXEL: {
         auto mask = cb_shader_mask.value >> (4 * exp->arrayBase);

         if (!mask) {
            gLog->warn("Export is masked by cb_shader_mask");
         } else {
            out
               << "ps_out_"
               << exp->arrayBase
               << " = exp_pixel_"
               << exp->arrayBase << '.';

            if (mask & (1 << 0)) {
               out << 'x';
            }

            if (mask & (1 << 1)) {
               out << 'y';
            }

            if (mask & (1 << 2)) {
               out << 'z';
            }

            if (mask & (1 << 3)) {
               out << 'w';
            }

            out << ";\n";
         }
      } break;
      case latte::SQ_EXPORT_POS:
         throw std::logic_error("Unexpected position export in pixel shader.");
         break;
      case latte::SQ_EXPORT_PARAM:
         throw std::logic_error("Unexpected parameter export in pixel shader.");
         break;
      }
   }

   out << "}\n";

   out << "/*\n" << pixel.disassembly << "\n*/\n";

   pixel.code = out.str();
   return true;
}

} // namespace opengl

} // namespace gpu
