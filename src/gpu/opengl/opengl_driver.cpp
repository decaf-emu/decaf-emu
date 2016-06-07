#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <glbinding/Meta.h>
#include <gsl.h>
#include <fstream>

#include "platform/platform_ui.h"
#include "gpu/commandqueue.h"
#include "gpu/latte_registers.h"
#include "gpu/pm4_buffer.h"
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_event.h"
#include "opengl_driver.h"
#include "common/log.h"

namespace gpu
{

namespace opengl
{

void GLDriver::initGL()
{
   platform::ui::activateContext();

   glbinding::Binding::initialize();

   glbinding::setCallbackMaskExcept(glbinding::CallbackMask::After | glbinding::CallbackMask::ParametersAndReturnValue, { "glGetError" });
   glbinding::setAfterCallback([](const glbinding::FunctionCall &call) {
      auto error = glbinding::Binding::GetError.directCall();

      if (error != gl::GL_NO_ERROR) {
         fmt::MemoryWriter writer;
         writer << call.function->name() << "(";

         for (unsigned i = 0; i < call.parameters.size(); ++i) {
            writer << call.parameters[i]->asString();
            if (i < call.parameters.size() - 1)
               writer << ", ";
         }

         writer << ")";

         if (call.returnValue) {
            writer << " -> " << call.returnValue->asString();
         }

         gLog->error("OpenGL error: {} with {}", glbinding::Meta::getString(error), writer.str());
      }
   });

   // Set a background color for emu window.
   gl::glClearColor(0.6f, 0.2f, 0.2f, 1.0f);
   platform::ui::bindTvWindow();
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
   platform::ui::bindDrcWindow();
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
   // Sneakily assume double-buffering...
   platform::ui::swapBuffers();
   platform::ui::bindTvWindow();
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);
   platform::ui::bindDrcWindow();
   gl::glClear(gl::GL_COLOR_BUFFER_BIT);

   // Clear active state
   mRegisters.fill(0);
   mActiveShader = nullptr;
   mActiveDepthBuffer = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(ColorBuffer *) * mActiveColorBuffers.size());
   std::memset(&mPendingEOP, 0, sizeof(pm4::EventWriteEOP));

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer.object);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);

   static auto vertexCode = R"(
      #version 420 core
      in vec2 fs_position;
      in vec2 fs_texCoord;
      out vec2 vs_texCoord;

      out gl_PerVertex {
         vec4 gl_Position;
      };

      void main()
      {
         vs_texCoord = fs_texCoord;
         gl_Position = vec4(fs_position, 0.0, 1.0);
      })";

   static auto pixelCode = R"(
      #version 420 core
      in vec2 vs_texCoord;
      out vec4 ps_color;
      uniform sampler2D sampler_0;

      void main()
      {
         ps_color = texture(sampler_0, vs_texCoord);
      })";

   // Create vertex program
   mScreenDraw.vertexProgram = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, &vertexCode);

   // Create pixel program
   mScreenDraw.pixelProgram = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, &pixelCode);
   gl::glBindFragDataLocation(mScreenDraw.pixelProgram, 0, "ps_color");

   // Create pipeline
   gl::glGenProgramPipelines(1, &mScreenDraw.pipeline);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_VERTEX_SHADER_BIT, mScreenDraw.vertexProgram);
   gl::glUseProgramStages(mScreenDraw.pipeline, gl::GL_FRAGMENT_SHADER_BIT, mScreenDraw.pixelProgram);

   // (TL, TR, BR)    (BR, BL, TL)
   // Create vertex buffer
   static const gl::GLfloat vertices[] = {
      -1.0f,  1.0f,   0.0f, 1.0f,
       1.0f,  1.0f,   1.0f, 1.0f,
       1.0f, -1.0f,   1.0f, 0.0f,

       1.0f, -1.0f,   1.0f, 0.0f,
      -1.0f, -1.0f,   0.0f, 0.0f,
      -1.0f,  1.0f,   0.0f, 1.0f,
   };

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

enum
{
   SCANTARGET_TV = 1,
   SCANTARGET_DRC = 4,
};

void GLDriver::decafCopyColorToScan(const pm4::DecafCopyColorToScan &data)
{
   auto cb_color_base = bit_cast<latte::CB_COLORN_BASE>(data.bufferAddr);
   auto buffer = getColorBuffer(cb_color_base, data.cb_color_size, data.cb_color_info);

   // Unbind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);

   // Bind appropriate window and set viewport
   if (data.scanTarget == SCANTARGET_TV) {
      platform::ui::bindTvWindow();
   } else if (data.scanTarget == SCANTARGET_DRC) {
      platform::ui::bindDrcWindow();
   } else {
      gLog->error("decafCopyColorToScan called for unknown scanTarget.");
   }

   // Setup screen draw shader
   gl::glBindVertexArray(mScreenDraw.vertArray);
   gl::glBindVertexBuffer(0, mScreenDraw.vertBuffer, 0, 4 * sizeof(gl::GLfloat));
   gl::glBindProgramPipeline(mScreenDraw.pipeline);

   // Set active shader to nullptr so it has to rebind.
   mActiveShader = nullptr;

   // Draw screen quad
   gl::glColorMaski(0, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE, gl::GL_TRUE);
   gl::glBindSampler(0, 0);
   gl::glDisablei(gl::GL_BLEND, 0);
   gl::glDisable(gl::GL_DEPTH_TEST);
   gl::glDisable(gl::GL_STENCIL_TEST);
   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glDisable(gl::GL_CULL_FACE);
   gl::glDisable(gl::GL_ALPHA_TEST);
   gl::glBindTextureUnit(0, buffer->object);

   gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);

   // Rebind active framebuffer
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer.object);
}

void GLDriver::decafSwapBuffers(const pm4::DecafSwapBuffers &data)
{
   static const auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds { 1 }).count();
   static const auto weight = 0.9;

   platform::ui::swapBuffers();
   gx2::internal::onFlip();

   auto now = std::chrono::system_clock::now();

   if (mLastSwap.time_since_epoch().count()) {
      mAverageFrameTime = weight * mAverageFrameTime + (1.0 - weight) * (now - mLastSwap);

      auto fps = second / mAverageFrameTime.count();
      auto time = std::chrono::duration_cast<duration_ms>(mAverageFrameTime);
      platform::ui::setTvTitle(fmt::format("Decaf - FPS: {:.2f} Frame Time: {:.2f}", fps, time.count()));
   }

   mLastSwap = now;
}

void GLDriver::decafSetContextState(const pm4::DecafSetContextState &data)
{
   mContextState = reinterpret_cast<latte::ContextState *>(data.context.get());
}

uint64_t GLDriver::getGpuClock()
{
   return coreinit::OSGetTime();
}

void GLDriver::memWrite(const pm4::MemWrite &data)
{
   uint64_t value;
   auto addr = mem::translate(data.addrLo.ADDR_LO() << 2);

   if (data.addrHi.CNTR_SEL() == pm4::MW_WRITE_CLOCK) {
      value = getGpuClock();
   } else {
      value = static_cast<uint64_t>(data.dataLo) | static_cast<uint64_t>(data.dataHi) << 32;
   }

   switch (data.addrLo.ENDIAN_SWAP())
   {
   case latte::CB_ENDIAN_8IN64:
      value = byte_swap(value);
      break;
   case latte::CB_ENDIAN_8IN32:
      value = byte_swap(static_cast<uint32_t>(value));
      break;
   case latte::CB_ENDIAN_8IN16:
      throw std::logic_error("Unexpected MEM_WRITE endian swap");
   }

   if (data.addrHi.DATA32()) {
      *reinterpret_cast<uint32_t *>(addr) = static_cast<uint32_t>(value);
   } else {
      *reinterpret_cast<uint64_t *>(addr) = value;
   }
}

void GLDriver::eventWriteEOP(const pm4::EventWriteEOP &data)
{
   mPendingEOP = data;
}

void GLDriver::handlePendingEOP()
{
   if (!mPendingEOP.eventInitiator.EVENT_TYPE()) {
      return;
   }

   uint64_t value = 0;
   auto addr = mem::translate(mPendingEOP.addrLo.ADDR_LO() << 2);

   switch (mPendingEOP.eventInitiator.EVENT_TYPE()) {
   case latte::BOTTOM_OF_PIPE_TS:
      value = getGpuClock();
      break;
   default:
      throw std::logic_error("Unexpected EOP event type");
   }

   switch (mPendingEOP.addrLo.ENDIAN_SWAP()) {
   case latte::CB_ENDIAN_8IN64:
      value = byte_swap(value);
      break;
   case latte::CB_ENDIAN_8IN32:
      value = byte_swap(static_cast<uint32_t>(value));
      break;
   case latte::CB_ENDIAN_8IN16:
      throw std::logic_error("Unexpected EOP event endian swap");
   }

   switch (mPendingEOP.addrHi.DATA_SEL()) {
   case pm4::EW_DATA_32:
      *reinterpret_cast<uint32_t *>(addr) = static_cast<uint32_t>(value);
      break;
   case pm4::EW_DATA_64:
      *reinterpret_cast<uint64_t *>(addr) = value;
      break;
   }

   std::memset(&mPendingEOP, 0, sizeof(pm4::EventWriteEOP));
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

void GLDriver::run()
{
   initGL();

   while (mRunning) {
      auto buffer = gpu::unqueueCommandBuffer();

      // Execute command buffer
      runCommandBuffer(buffer->buffer, buffer->curSize);

      // Handle end-of-pipeline events
      handlePendingEOP();

      // Release command buffer
      gpu::retireCommandBuffer(buffer);
   }
}

} // namespace opengl

} // namespace gpu
