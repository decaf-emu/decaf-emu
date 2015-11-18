#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

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

// We need LOAD_REG and SET_REG to do same shit.

bool GLDriver::checkActiveColorBuffer()
{
   for (auto i = 0u; i < 8; ++i) {
      auto cb_color_base = getRegister<latte::CB_COLORN_BASE>(latte::Register::CB_COLOR0_BASE + i * 4);
      auto &active = mActiveColorBuffers[i];

      if (!cb_color_base.BASE_256B) {
         if (active) {
            // Unbind active
            gl::glFramebufferRenderbuffer(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, gl::GL_RENDERBUFFER, 0);
            active = nullptr;
         }

         continue;
      }

      if (active && active->cb_color_base.BASE_256B == cb_color_base.BASE_256B) {
         // Already bound
         continue;
      }

      if (!active->object) {
         auto cb_color_size = getRegister<latte::CB_COLORN_SIZE>(latte::Register::CB_COLOR0_SIZE + i * 4);
         auto cb_color_info = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_INFO + i * 4);
         auto cb_color_tile = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_TILE + i * 4);
         auto cb_color_frag = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_FRAG + i * 4);
         auto cb_color_view = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_VIEW + i * 4);
         auto cb_color_mask = getRegister<latte::CB_COLORN_INFO>(latte::Register::CB_COLOR0_MASK + i * 4);

         auto format = cb_color_info.FORMAT;
         auto pitch_tile_max = cb_color_size.PITCH_TILE_MAX;
         auto slice_tile_max = cb_color_size.SLICE_TILE_MAX;

         auto pitch = (pitch_tile_max + 1) * latte::tile_width;
         auto height = ((slice_tile_max + 1) * (latte::tile_width * latte::tile_height)) / pitch;

         // Create render buffer
         active = &mColorBuffers[cb_color_base.BASE_256B];
         gl::glGenRenderbuffers(1, &active->object);
         gl::glBindRenderbuffer(gl::GL_RENDERBUFFER, active->object);

         // TODO: Use colorbuffer format
         gl::glRenderbufferStorage(gl::GL_RENDERBUFFER, gl::GL_RGBA, pitch, height);
      }

      // Bind renderbuffer to framebuffer
      gl::glFramebufferRenderbuffer(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0 + i, gl::GL_RENDERBUFFER, active->object);
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

   // Genearte shader if needed
   if (!shader.object) {
      // Parse fetch shader if needed
      if (!fetchShader.parsed) {
         auto program = make_virtual_ptr<void>(pgm_start_fs.PGM_START << 8);
         auto size = pgm_size_fs.PGM_SIZE << 3;

         if (!parseFetchShader(fetchShader, program, size)) {
            gLog->error("Failed to parse fetch shader");
            return false;
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

         if (!vertexShader.object) {
            gl::GLint logLength = 0;
            std::string logMessage;
            gl::glGetProgramiv(vertexShader.object, gl::GL_INFO_LOG_LENGTH, &logLength);

            logMessage.resize(logLength);
            gl::glGetProgramInfoLog(vertexShader.object, logLength, &logLength, &logMessage[0]);
            gLog->error("Failed to compile vertex shader glsl:\n{}", logMessage);
            return false;
         }
      }

      // Compile pixel shader if needed
      if (!pixelShader.object) {
         auto program = make_virtual_ptr<uint8_t>(pgm_start_ps.PGM_START << 8);
         auto size = pgm_size_ps.PGM_SIZE << 3;

         if (!compilePixelShader(pixelShader, program, size)) {
            gLog->error("Failed to compile vertex shader");
            return false;
         }

         // Create OpenGL Shader
         const gl::GLchar *code[] = { vertexShader.code.c_str() };
         pixelShader.object = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, code);

         if (!pixelShader.object) {
            gl::GLint logLength = 0;
            std::string logMessage;
            gl::glGetProgramiv(pixelShader.object, gl::GL_INFO_LOG_LENGTH, &logLength);

            logMessage.resize(logLength);
            gl::glGetProgramInfoLog(pixelShader.object, logLength, &logLength, &logMessage[0]);
            gLog->error("Failed to compile pixel shader glsl:\n{}", logMessage);
            return false;
         }
      }

      if (fetchShader.parsed && vertexShader.object && pixelShader.object) {
         shader.fetch = &fetchShader;
         shader.vertex = &vertexShader;
         shader.pixel = &pixelShader;

         // Create pipeline
         gl::glGenProgramPipelines(1, &shader.object);
         gl::glUseProgramStages(shader.object, gl::GL_VERTEX_SHADER_BIT, shader.vertex->object);
         gl::glUseProgramStages(shader.object, gl::GL_FRAGMENT_SHADER_BIT, shader.pixel->object);
      }
   }

   // Bind shader
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

void GLDriver::decafCopyColorToScan(pm4::DecafCopyColorToScan &data)
{
}

void GLDriver::decafSwapBuffers(pm4::DecafSwapBuffers &data)
{
   swapBuffers();
}

void GLDriver::decafClearColor(pm4::DecafClearColor &data)
{
   // Check if data.buffer is bound to 0-8 active, if so clear N, if not bind to 0 and clear 0
   //gl::glClearBufferiv(gl::GL_COLOR, )
   // TODO: Set current color buffer
   gl::glClearColor(data.red, data.green, data.blue, data.alpha);
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
}

void GLDriver::decafClearDepthStencil(pm4::DecafClearDepthStencil &data)
{
}

void GLDriver::drawIndexAuto(pm4::DrawIndexAuto &data)
{
   if (!checkActiveShader()) {
      gLog->warn("Skipping draw with invalid shader.");
      return;
   }
}

void GLDriver::drawIndex2(pm4::DrawIndex2 &data)
{
   if (!checkActiveShader()) {
      gLog->warn("Skipping draw with invalid shader.");
      return;
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
   for (auto i = 0u; i < data.values.size(); ++i) {
      setRegister(static_cast<latte::Register::Value>(data.id + i * 4), data.values[i]);
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

void GLDriver::runCommandBuffer(uint32_t *buffer, uint32_t size)
{
   for (auto pos = 0u; pos < size; ) {
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
   activateDeviceContext();
   glbinding::Binding::initialize();

   mActiveShader = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(ColorBuffer *) * mActiveColorBuffers.size());

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer.object);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);

   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();

      runCommandBuffer(buffer->buffer, buffer->curSize);

      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
