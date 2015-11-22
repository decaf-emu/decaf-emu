#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>
#include <fstream>

#include "platform/platform_ui.h"
#include "gpu/commandqueue.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_reader.h"
#include "gpu/latte_registers.h"
#include "gpu/latte_untile.h"
#include "opengl_driver.h"
#include "utils/log.h"

namespace gpu
{

namespace opengl
{

bool GLDriver::checkReadyDraw()
{
   if (!checkActiveShader()) {
      gLog->warn("Skipping draw with invalid shader.");
      return false;
   }

   if (!checkActiveAttribBuffers()) {
      gLog->warn("Skipping draw with invalid attribs.");
      return false;
   }

   if (!checkActiveUniforms()) {
      gLog->warn("Skipping draw with invalid uniforms.");
      return false;
   }

   if (!checkActiveColorBuffer()) {
      gLog->warn("Skipping draw with invalid color buffer.");
      return false;
   }

   if (!checkActiveDepthBuffer()) {
      gLog->warn("Skipping draw with invalid depth buffer.");
      return false;
   }

   return true;
}

ColorBuffer *
GLDriver::getColorBuffer(latte::CB_COLORN_BASE &cb_color_base,
                         latte::CB_COLORN_SIZE &cb_color_size,
                         latte::CB_COLORN_INFO &cb_color_info)
{
   auto buffer = &mColorBuffers[cb_color_base.BASE_256B];
   buffer->cb_color_base = cb_color_base;

   if (!buffer->object) {
      auto format = cb_color_info.FORMAT;
      auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX;
      auto slice_tile_max = cb_color_size.SLICE_TILE_MAX;

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::tile_width);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::tile_width * latte::tile_height)) / pitch);

      // Create color buffer
      gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureStorage2D(buffer->object, 1, gl::GL_RGBA8, pitch, height);
   }

   return buffer;
}

DepthBuffer *
GLDriver::getDepthBuffer(latte::DB_DEPTH_BASE &db_depth_base,
                         latte::DB_DEPTH_SIZE &db_depth_size,
                         latte::DB_DEPTH_INFO &db_depth_info)
{
   auto buffer = &mDepthBuffers[db_depth_base.BASE_256B];
   buffer->db_depth_base = db_depth_base;

   if (!buffer->object) {
      auto format = db_depth_info.FORMAT;
      auto pitch_tile_max = db_depth_size.PITCH_TILE_MAX;
      auto slice_tile_max = db_depth_size.SLICE_TILE_MAX;

      auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::tile_width);
      auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::tile_width * latte::tile_height)) / pitch);

      // Create depth buffer
      gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &buffer->object);
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureParameteri(buffer->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
      gl::glTextureStorage2D(buffer->object, 1, gl::GL_DEPTH_COMPONENT32F, pitch, height);
   }

   return buffer;
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
   default:
      throw std::logic_error("Unsupported attribute format");
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
   case latte::FMT_8_8_8_8:
   case latte::FMT_16_16_16_16:
   case latte::FMT_16_16_16_16_FLOAT:
   case latte::FMT_32_32_32_32:
   case latte::FMT_32_32_32_32_FLOAT:
      return 4;
   default:
      throw std::logic_error("Unsupported attribute format");
      return 4;
   }
}

template<typename Type, int N>
void stridedMemcpy2(void *srcBuffer, void *dstBuffer, size_t size, size_t offset, size_t stride, bool endian)
{
   auto src = reinterpret_cast<uint8_t *>(srcBuffer) + offset;
   auto dst = reinterpret_cast<uint8_t *>(dstBuffer) + offset;
   auto end = reinterpret_cast<uint8_t *>(srcBuffer) + size;

   if (endian) {
      while (src < end) {
         auto srcPtr = reinterpret_cast<Type *>(src);
         auto dstPtr = reinterpret_cast<Type *>(dst);

         for (auto i = 0u; i < N; ++i) {
            *dstPtr++ = byte_swap(*srcPtr++);
         }

         src += stride;
         dst += stride;
      }
   } else {
      while (src < end) {
         memcpy(src, dst, sizeof(Type) * N);
         src += stride;
         dst += stride;
      }
   }
}

void stridedMemcpy(void *src, void *dst, size_t size, size_t offset, size_t stride, latte::SQ_ENDIAN endian, latte::SQ_DATA_FORMAT format)
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
      throw std::logic_error("Invalid strided memcpy format");
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

      if (sq_vtx_constant_word6.TYPE != latte::SQ_TEX_VTX_VALID_BUFFER) {
         gLog->error("No valid buffer set for attrib resource {}", index);
         return false;
      }

      auto addr = sq_vtx_constant_word0.BASE_ADDRESS;
      auto size = sq_vtx_constant_word1.SIZE + 1;
      auto stride = sq_vtx_constant_word2.STRIDE;
      auto &buffer = mAttribBuffers[addr];

      if (!buffer.object) {
         buffer.size = size;
         buffer.stride = stride;

         gl::glCreateBuffers(1, &buffer.object);
         gl::glNamedBufferStorage(buffer.object, size, NULL, gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
         buffer.mappedBuffer = gl::glMapNamedBufferRange(buffer.object, 0, size, gl::GL_MAP_FLUSH_EXPLICIT_BIT | gl::GL_MAP_WRITE_BIT | gl::GL_MAP_PERSISTENT_BIT);
      } else if (buffer.size != size || buffer.stride != stride) {
         throw std::logic_error("Buffer size has changed!");
      }

      stridedMemcpy(make_virtual_ptr<void *>(addr),
                    buffer.mappedBuffer,
                    buffer.size,
                    attrib.offset,
                    buffer.stride,
                    attrib.endianSwap,
                    attrib.format);

      // TODO: Only flush + bind once per buffer
      gl::glFlushMappedNamedBufferRange(buffer.object, 0, buffer.size);
      gl::glBindVertexBuffer(attrib.buffer, buffer.object, 0, buffer.stride);
   }

   return true;
}

bool GLDriver::checkActiveColorBuffer()
{
   for (auto i = 0u; i < mActiveColorBuffers.size(); ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto &active = mActiveColorBuffers[i];

      if (!cb_color_base.BASE_256B) {
         if (active) {
            // Unbind active
            gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, 0, 0);
            active = nullptr;
         }

         continue;
      }

      if (active && active->cb_color_base.BASE_256B == cb_color_base.BASE_256B) {
         // Already bound
         continue;
      }

      auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
      auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
      active = getColorBuffer(cb_color_base, cb_color_size, cb_color_info);

      // Bind color buffer
      gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, active->object, 0);
   }

   return true;
}

bool GLDriver::checkActiveDepthBuffer()
{
   auto db_depth_base = getRegister<latte::DB_DEPTH_BASE>(latte::Register::DB_DEPTH_BASE);
   auto &active = mActiveDepthBuffer;

   if (!db_depth_base.BASE_256B) {
      if (active) {
         // Unbind active
         gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, 0, 0);
         active = nullptr;
      }

      return true;
   }

   if (active && active->db_depth_base.BASE_256B == db_depth_base.BASE_256B) {
      // Already bound
      return true;
   }

   auto db_depth_size = getRegister<latte::DB_DEPTH_SIZE>(latte::Register::DB_DEPTH_SIZE);
   auto db_depth_info = getRegister<latte::DB_DEPTH_INFO>(latte::Register::DB_DEPTH_INFO);
   active = getDepthBuffer(db_depth_base, db_depth_size, db_depth_info);

   // Bind depth buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_DEPTH_ATTACHMENT, active->object, 0);
   return true;
}

bool GLDriver::checkActiveUniforms()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);

   if (!mActiveShader) {
      return true;
   }

   if (sq_config.DX9_CONSTS) {
      // Upload uniform registers
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_256 / 4]);
         gl::glProgramUniform4fv(mActiveShader->vertex->object, mActiveShader->vertex->uniformRegisters, 256 * 4, values);
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_0 / 4]);
         gl::glProgramUniform4fv(mActiveShader->pixel->object, mActiveShader->pixel->uniformRegisters, 256 * 4, values);
      }
   } else {
      // TODO: Upload uniform blocks
   }

   return true;
}

bool GLDriver::checkActiveShader()
{
   auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
   auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
   auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
   auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);

   if (mActiveShader &&
       mActiveShader->fetch && mActiveShader->fetch->pgm_start_fs.PGM_START != pgm_start_fs.PGM_START
       && mActiveShader->vertex && mActiveShader->vertex->pgm_start_vs.PGM_START != pgm_start_vs.PGM_START
       && mActiveShader->pixel && mActiveShader->pixel->pgm_start_ps.PGM_START != pgm_start_ps.PGM_START) {
      // OpenGL shader matches latte shader
      return true;
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
            auto normalise = attrib.numFormat == latte::SQ_NUM_FORMAT_SCALED ? gl::GL_TRUE : gl::GL_FALSE;
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

         // Check program log
         auto log = getProgramLog(vertexShader.object, gl::glGetProgramiv, gl::glGetProgramInfoLog);
         if (log.size()) {
            gLog->error("OpenGL failed to compile vertex shader:\n{}", log);
            gLog->error("Shader Code:\n{}\n", vertexShader.code);
            return false;
         }

         // Get uniform locations
         vertexShader.uniformRegisters = gl::glGetUniformLocation(vertexShader.object, "VC");

         // Get attribute locations
         vertexShader.attribLocations.fill(0);

         for (auto &attrib : fetchShader.attribs) {
            char name[32];
            sprintf_s(name, 32, "fs_out_%u", attrib.location);
            vertexShader.attribLocations[attrib.location] = gl::glGetAttribLocation(vertexShader.object, name);
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

         // Check program log
         auto log = getProgramLog(pixelShader.object, gl::glGetProgramiv, gl::glGetProgramInfoLog);
         if (log.size()) {
            gLog->error("OpenGL failed to compile pixel shader:\n{}", log);
            gLog->error("Shader Code:\n{}\n", pixelShader.code);
            return false;
         }

         // Get uniform locations
         pixelShader.uniformRegisters = gl::glGetUniformLocation(pixelShader.object, "VC");
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

void GLDriver::setRegister(latte::Register::Value reg, uint32_t value)
{
   // Save to my state
   mRegisters[reg / 4] = value;

   // TODO: Save to active context state shadow regs

   // For the following registers, we apply their state changes
   //   directly to the OpenGL context...
   switch (reg) {
   case latte::Register::SQ_VTX_SEMANTIC_CLEAR:
      for (auto i = 0u; i < 32; ++i) {
         setRegister(static_cast<latte::Register::Value>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4), 0xffffffff);
      }
      break;
   case latte::Register::CB_BLEND_CONTROL:
   case latte::Register::CB_BLEND0_CONTROL:
   case latte::Register::CB_BLEND1_CONTROL:
   case latte::Register::CB_BLEND2_CONTROL:
   case latte::Register::CB_BLEND3_CONTROL:
   case latte::Register::CB_BLEND4_CONTROL:
   case latte::Register::CB_BLEND5_CONTROL:
   case latte::Register::CB_BLEND6_CONTROL:
   case latte::Register::CB_BLEND7_CONTROL:
      // gl::something();
      break;
   }
}

static std::string
readFileToString(const std::string &filename)
{
   std::ifstream in { filename, std::ifstream::binary };
   std::string result;

   if (in.is_open()) {
      in.seekg(0, in.end);
      auto size = in.tellg();
      result.resize(size);
      in.seekg(0, in.beg);
      in.read(&result[0], size);
   }

   return result;
}

void GLDriver::initGL()
{
   mRegisters.fill(0);
   activateDeviceContext();
   glbinding::Binding::initialize();

   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
      auto error = gl::glGetError();

      if (error != gl::GL_NO_ERROR) {
         gLog->error("OpenGL {} error: {}", call.toString(), error);
      }
   });

   // Set a background color for emu window.
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
   // Sneakily assume double-buffering...
   swapBuffers();
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Clear active state
   mActiveShader = nullptr;
   mActiveDepthBuffer = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(ColorBuffer *) * mActiveColorBuffers.size());

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer.object);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);

   auto vertexCode = readFileToString("resources/shaders/screen_vertex.glsl");
   if (!vertexCode.size()) {
      gLog->error("Could not load resources/shaders/screen_vertex.glsl");
   }

   auto pixelCode = readFileToString("resources/shaders/screen_pixel.glsl");
   if (!pixelCode.size()) {
      gLog->error("Could not load resources/shaders/screen_pixel.glsl");
   }

   // Create vertex program
   auto code = vertexCode.c_str();
   mScreenDraw.vertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &code);

   // Create pixel program
   code = pixelCode.c_str();
   mScreenDraw.pixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &code);
   gl::glBindFragDataLocation(mScreenDraw.pixelProgram, 0, "ps_color");

   gl::GLint logLength = 0;
   std::string logMessage;
   gl::glGetProgramiv(mScreenDraw.vertexProgram, gl::GL_INFO_LOG_LENGTH, &logLength);

   logMessage.resize(logLength);
   gl::glGetProgramInfoLog(mScreenDraw.vertexProgram, logLength, &logLength, &logMessage[0]);
   gLog->error("Failed to compile vertex shader glsl:\n{}", logMessage);

   // Create pipeline
   gl::glGenProgramPipelines(1, &mScreenDraw.pipeline);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_VERTEX_SHADER_BIT, mScreenDraw.vertexProgram);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_FRAGMENT_SHADER_BIT, mScreenDraw.pixelProgram);

   float wndWidth = static_cast<float>(platform::ui::getWindowWidth());
   float wndHeight = static_cast<float>(platform::ui::getWindowHeight());
   float tvWidth = static_cast<float>(platform::ui::getTvWidth());
   float tvHeight = static_cast<float>(platform::ui::getTvHeight());
   float drcWidth = static_cast<float>(platform::ui::getDrcWidth());
   float drcHeight = static_cast<float>(platform::ui::getDrcHeight());

   auto tvXHeight = wndHeight * 0.7f;
   auto tvXWidth = tvWidth * (tvXHeight / tvHeight);
   auto drcXHeight = wndHeight - tvXHeight;
   auto drcXWidth = drcWidth * (drcXHeight / drcHeight);

   auto tvTop = 0;
   auto tvLeft = (wndWidth - tvXWidth) / 2;
   auto tvBottom = tvTop + tvXHeight;
   auto tvRight = tvLeft + tvXWidth;
   auto drcTop = tvBottom;
   auto drcLeft = (wndWidth - drcXWidth) / 2;
   auto drcBottom = drcTop + drcXHeight;
   auto drcRight = drcLeft + drcXWidth;

#define SX(x) (((x)/wndWidth*2)-1)
#define SY(y) -(((y)/wndHeight*2)-1)

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      // TV
      SX(tvLeft),  SY(tvTop),       0.0f, 1.0f,
      SX(tvRight), SY(tvTop),       1.0f, 1.0f,
      SX(tvRight), SY(tvBottom),    1.0f, 0.0f,

      SX(tvRight), SY(tvBottom),    1.0f, 0.0f,
      SX(tvLeft),  SY(tvBottom),    0.0f, 0.0f,
      SX(tvLeft),  SY(tvTop),       0.0f, 1.0f,

      // DRC
      SX(drcLeft),  SY(drcTop),     0.0f, 1.0f,
      SX(drcRight), SY(drcTop),     1.0f, 1.0f,
      SX(drcRight), SY(drcBottom),  1.0f, 0.0f,

      SX(drcRight), SY(drcBottom),  1.0f, 0.0f,
      SX(drcLeft),  SY(drcBottom),  0.0f, 0.0f,
      SX(drcLeft),  SY(drcTop),     0.0f, 1.0f
   };
#undef SX
#undef SY

   gl::glCreateBuffers(1, &mScreenDraw.vertBuffer);
   gl::glNamedBufferData(mScreenDraw.vertBuffer, sizeof(vertices), vertices, gl::GL_STATIC_DRAW);

   // Create vertex array
   gl::glCreateVertexArrays(1, &mScreenDraw.vertArray);

   auto fs_position = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_position");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_position);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_position, 2, gl::GL_FLOAT, gl::GL_FALSE, 0);
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_position, 0);

   auto fs_texCoord = gl::glGetAttribLocation(mScreenDraw.vertexProgram, "fs_texCoord");
   gl::glEnableVertexArrayAttrib(mScreenDraw.vertArray, fs_texCoord);
   gl::glVertexArrayAttribFormat(mScreenDraw.vertArray, fs_texCoord, 2, gl::GL_FLOAT, gl::GL_FALSE, 2 * sizeof(gl::GLfloat));
   gl::glVertexArrayAttribBinding(mScreenDraw.vertArray, fs_texCoord, 0);
}

enum {
   SCANTARGET_TV = 1,
   SCANTARGET_DRC = 4,
};
void GLDriver::decafCopyColorToScan(pm4::DecafCopyColorToScan &data)
{
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Unbind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);

   // Setup screen draw shader
   gl::glBindVertexArray(mScreenDraw.vertArray);
   gl::glBindVertexBuffer(0, mScreenDraw.vertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mScreenDraw.pipeline);

   // Draw screen quad
   gl::glEnable(gl::GL_TEXTURE_2D);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glActiveTexture(gl::GL_TEXTURE0);
   gl::glBindTexture(gl::GL_TEXTURE_2D, buffer->object);

   if (data.scanTarget == SCANTARGET_TV) {
      gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
   } else if (data.scanTarget == SCANTARGET_DRC) {
      gl::glDrawArrays(gl::GL_TRIANGLES, 6, 6);
   } else {
      gLog->error("decafCopyColorToScan called for unknown scanTarget.");
   }

   // Rebind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);
}

void GLDriver::decafSwapBuffers(pm4::DecafSwapBuffers &data)
{
   swapBuffers();
}

void GLDriver::decafClearColor(pm4::DecafClearColor &data)
{
   float colors[] = {
      data.red,
      data.green,
      data.blue,
      data.alpha
   };

   // Check if the color buffer is actively bound
   for (auto i = 0; i < 8; ++i) {
      auto active = mActiveColorBuffers[i];

      if (!active) {
         continue;
      }

      if (active->cb_color_base.BASE_256B == data.bufferAddr) {
         gl::glClearBufferfv(gl::GL_COLOR, i, colors);
         return;
      }
   }

   // Find our colorbuffer to clear
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Temporarily set to this color buffer
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, buffer->object, 0);

   // Clear the buffer
   gl::glClearBufferfv(gl::GL_COLOR, 0, colors);

   // Clear the temporary color buffer attachement
   mActiveColorBuffers[0] = nullptr;
   gl::glFramebufferTexture(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0, 0, 0);
}

void GLDriver::decafClearDepthStencil(pm4::DecafClearDepthStencil &data)
{
}

gl::GLenum getGlPrimitiveType(latte::VGT_DI_PRIMITIVE_TYPE primType)
{
   gl::GLenum mode;
   switch (primType) {
   case latte::VGT_DI_PT_POINTLIST:
      mode = gl::GL_POINTS;
      break;
   case latte::VGT_DI_PT_LINELIST:
      mode = gl::GL_LINES;
      break;
   case latte::VGT_DI_PT_LINESTRIP:
      mode = gl::GL_LINE_STRIP;
      break;
   case latte::VGT_DI_PT_TRILIST:
      mode = gl::GL_TRIANGLES;
      break;
   case latte::VGT_DI_PT_TRIFAN:
      mode = gl::GL_TRIANGLE_FAN;
      break;
   case latte::VGT_DI_PT_TRISTRIP:
      mode = gl::GL_TRIANGLE_STRIP;
      break;
   case latte::VGT_DI_PT_LINELOOP:
      mode = gl::GL_LINE_LOOP;
      break;
   default:
      throw std::logic_error("Invalid VGT_PRIMITIVE_TYPE");
   }
   return mode;
}

template<typename IndexType>
void drawPrimitives3(gl::GLenum mode, uint32_t offset, uint32_t count, const IndexType *indices)
{
   if (!indices) {
      gl::glDrawArrays(mode, offset, count);
   } else {
      if (std::is_same<IndexType, uint16_t>()) {
         gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_SHORT, indices, offset);
      } else if (std::is_same<IndexType, uint32_t>()) {
         gl::glDrawElementsBaseVertex(mode, count, gl::GL_UNSIGNED_INT, indices, offset);
      } else {
         throw new std::logic_error("Unexpected index type.");
      }
   }
}

template<typename IndexType>
void drawQuadList(uint32_t offset, uint32_t count, const IndexType *indices)
{
   auto triCount = count * 6 / 4;
   auto quadIndices = new IndexType[triCount];

   auto indicesOut = quadIndices;
   for (auto i = 0u; i < count / 4; ++i) {
      auto index_tl = *indices++;
      auto index_tr = *indices++;
      auto index_br = *indices++;
      auto index_bl = *indices++;

      *indicesOut++ = index_tl;
      *indicesOut++ = index_tr;
      *indicesOut++ = index_bl;
      *indicesOut++ = index_bl;
      *indicesOut++ = index_tr;
      *indicesOut++ = index_br;
   }

   drawPrimitives3(gl::GL_TRIANGLES, offset, triCount, quadIndices);
   delete quadIndices;
}

template<typename IndexType>
void drawPrimitives2(latte::VGT_DI_PRIMITIVE_TYPE primType, uint32_t offset,
   uint32_t count, const IndexType *indices)
{
   if (primType == latte::VGT_DI_PT_QUADLIST) {
      return drawQuadList(offset, count, indices);
   } else {
      gl::GLenum mode = getGlPrimitiveType(primType);
      return drawPrimitives3(mode, offset, count, indices);
   }
}

void drawPrimitives(latte::VGT_DI_PRIMITIVE_TYPE primType, uint32_t offset, 
                    uint32_t count, const void *indices, latte::VGT_INDEX indexFmt)
{
   if (indexFmt == latte::VGT_INDEX_16) {
      drawPrimitives2(primType, offset, count, static_cast<const uint16_t*>(indices));
   } else if (indexFmt == latte::VGT_INDEX_32) {
      drawPrimitives2(primType, offset, count, static_cast<const uint32_t*>(indices));
   } else {
      throw new std::logic_error("Unexpected index format type.");
   }
}

void GLDriver::drawIndexAuto(pm4::DrawIndexAuto &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);

   drawPrimitives(vgt_primitive_type.PRIM_TYPE, sq_vtx_base_vtx_loc.OFFSET, 
      data.indexCount, nullptr, latte::VGT_INDEX_32);
}

void GLDriver::drawIndex2(pm4::DrawIndex2 &data)
{
   if (!checkReadyDraw()) {
      return;
   }

   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto sq_vtx_base_vtx_loc = getRegister<latte::SQ_VTX_BASE_VTX_LOC>(latte::Register::SQ_VTX_BASE_VTX_LOC);
   auto vgt_dma_index_type = getRegister<latte::VGT_DMA_INDEX_TYPE>(latte::Register::VGT_DMA_INDEX_TYPE);

   uint32_t indexBytes = 0;
   if (vgt_dma_index_type.INDEX_TYPE == latte::VGT_INDEX_16) {
      indexBytes = data.numIndices * 2;
   } else if (vgt_dma_index_type.INDEX_TYPE == latte::VGT_INDEX_32) {
      indexBytes = data.numIndices * 4;
   } else {
      throw new std::logic_error("Unexpected vgt_dma_index_type.INDEX_TYPE");
   }

   // Swap and indexBytes are separate because you can have 32-bit swap,
   //   but 16-bit indices in some cases...  This is also why we pre-swap
   //   the data before intercepting QUAD and POLYGON draws.
   if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_16_BIT) {
      auto *indicesIn = static_cast<uint16_t*>(data.addr.get());
      auto *indices = new uint16_t[indexBytes / sizeof(uint16_t)];
      for (auto i = 0u; i < data.numIndices; ++i) {
         indices[i] = byte_swap(indicesIn[i]);
      }
      drawPrimitives(vgt_primitive_type.PRIM_TYPE, sq_vtx_base_vtx_loc.OFFSET,
         data.numIndices, indices, vgt_dma_index_type.INDEX_TYPE);
      delete indices;
   } else if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_32_BIT) {
      auto *indicesIn = static_cast<uint32_t*>(data.addr.get());
      auto *indices = new uint32_t[indexBytes / sizeof(uint32_t)];
      for (auto i = 0u; i < data.numIndices; ++i) {
         indices[i] = byte_swap(indicesIn[i]);
      }
      drawPrimitives(vgt_primitive_type.PRIM_TYPE, sq_vtx_base_vtx_loc.OFFSET,
         data.numIndices, indices, vgt_dma_index_type.INDEX_TYPE);
      delete indices;
   } else if (vgt_dma_index_type.SWAP_MODE == latte::VGT_DMA_SWAP_NONE) {
      drawPrimitives(vgt_primitive_type.PRIM_TYPE, sq_vtx_base_vtx_loc.OFFSET,
         data.numIndices, data.addr, vgt_dma_index_type.INDEX_TYPE);
   }
}

void GLDriver::indexType(pm4::IndexType &data)
{
   mRegisters[latte::Register::VGT_DMA_INDEX_TYPE / 4] = data.type.value;
}

void GLDriver::numInstances(pm4::NumInstances &data)
{
   mRegisters[latte::Register::VGT_DMA_NUM_INSTANCES / 4] = data.count;
}

void GLDriver::setAluConsts(pm4::SetAluConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setConfigRegs(pm4::SetConfigRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setContextRegs(pm4::SetContextRegs &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setControlConstants(pm4::SetControlConstants &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setLoopConsts(pm4::SetLoopConsts &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setSamplers(pm4::SetSamplers &data)
{
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
   }
}

void GLDriver::setResources(pm4::SetResources &data)
{
   auto id = latte::Register::ResourceRegisterBase + (4 * data.id);

   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(id + i * 4), data.values[i]);
   }
}

void GLDriver::indirectBufferCall(pm4::IndirectBufferCall &data)
{
   auto buffer = reinterpret_cast<uint32_t*>(data.addr.get());
   runCommandBuffer(buffer, data.size);
}

void GLDriver::handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data)
{
   pm4::PacketReader reader { data };

   switch (header.opcode) {
   case pm4::Opcode3::DECAF_COPY_COLOR_TO_SCAN:
      decafCopyColorToScan(pm4::read<pm4::DecafCopyColorToScan>(reader));
      break;
   case pm4::Opcode3::DECAF_SWAP_BUFFERS:
      decafSwapBuffers(pm4::read<pm4::DecafSwapBuffers>(reader));
      break;
   case pm4::Opcode3::DECAF_CLEAR_COLOR:
      decafClearColor(pm4::read<pm4::DecafClearColor>(reader));
      break;
   case pm4::Opcode3::DECAF_CLEAR_DEPTH_STENCIL:
      decafClearDepthStencil(pm4::read<pm4::DecafClearDepthStencil>(reader));
      break;
   case pm4::Opcode3::DRAW_INDEX_AUTO:
      drawIndexAuto(pm4::read<pm4::DrawIndexAuto>(reader));
      break;
   case pm4::Opcode3::DRAW_INDEX_2:
      drawIndex2(pm4::read<pm4::DrawIndex2>(reader));
      break;
   case pm4::Opcode3::INDEX_TYPE:
      indexType(pm4::read<pm4::IndexType>(reader));
      break;
   case pm4::Opcode3::NUM_INSTANCES:
      numInstances(pm4::read<pm4::NumInstances>(reader));
      break;
   case pm4::Opcode3::SET_ALU_CONST:
      setAluConsts(pm4::read<pm4::SetAluConsts>(reader));
      break;
   case pm4::Opcode3::SET_CONFIG_REG:
      setConfigRegs(pm4::read<pm4::SetConfigRegs>(reader));
      break;
   case pm4::Opcode3::SET_CONTEXT_REG:
      setContextRegs(pm4::read<pm4::SetContextRegs>(reader));
      break;
   case pm4::Opcode3::SET_CTL_CONST:
      setControlConstants(pm4::read<pm4::SetControlConstants>(reader));
      break;
   case pm4::Opcode3::SET_LOOP_CONST:
      setLoopConsts(pm4::read<pm4::SetLoopConsts>(reader));
      break;
   case pm4::Opcode3::SET_SAMPLER:
      setSamplers(pm4::read<pm4::SetSamplers>(reader));
      break;
   case pm4::Opcode3::SET_RESOURCE:
      setResources(pm4::read<pm4::SetResources>(reader));
      break;
   case pm4::Opcode3::INDIRECT_BUFFER_PRIV:
      indirectBufferCall(pm4::read<pm4::IndirectBufferCall>(reader));
      break;
   }
}

void GLDriver::start()
{
   mRunning = true;
   mThread = std::thread(&GLDriver::run, this);
}

void GLDriver::setTvDisplay(size_t width, size_t height)
{
}

void GLDriver::setDrcDisplay(size_t width, size_t height)
{
}

void GLDriver::runCommandBuffer(uint32_t *buffer, uint32_t buffer_size)
{
   for (auto pos = 0u; pos < buffer_size; ) {
      auto header = *reinterpret_cast<pm4::PacketHeader *>(&buffer[pos]);
      auto size = 0u;

      switch (header.type) {
      case pm4::PacketType::Type3:
      {
         auto header3 = pm4::Packet3{ header.value };
         size = header3.size + 1;
         handlePacketType3(header3, { &buffer[pos + 1], size });
         break;
      }
      case pm4::PacketType::Type0:
      case pm4::PacketType::Type1:
      case pm4::PacketType::Type2:
      default:
         throw std::logic_error("What the fuck son");
      }

      pos += size + 1;
   }
}

void GLDriver::run()
{
   initGL();

   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();
      runCommandBuffer(buffer->buffer, buffer->curSize);
      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
