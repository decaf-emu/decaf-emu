#include "common/log.h"
#include "gpu/commandqueue.h"
#include "gpu/latte_registers.h"
#include "gpu/pm4_buffer.h"
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_event.h"
#include "modules/gx2/gx2_enum.h"
#include "opengl_driver.h"
#include <fstream>
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

namespace gpu
{

namespace opengl
{

void GLDriver::initGL()
{
   // Clear active state
   mRegisters.fill(0);
   mActiveShader = nullptr;
   mActiveDepthBuffer = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(SurfaceBuffer *) * mActiveColorBuffers.size());
   std::memset(&mPendingEOP, 0, sizeof(pm4::EventWriteEOP));

   // Create our blit framebuffer
   gl::glCreateFramebuffers(2, mBlitFrameBuffers);

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer);
}

void GLDriver::decafSetBuffer(const pm4::DecafSetBuffer &data)
{
   ScanBufferChain *chain = data.isTv ? &mTvScanBuffers : &mDrcScanBuffers;

   // Destroy any old chain
   if (chain->object) {
      gl::glDeleteTextures(1, &chain->object);
      chain->object = 0;
   }

   // NOTE: A SwapBufferChain is not really a chain as we rely on OpenGL
   //  to provide the buffering for us.  This means we do not always match
   //  the games buffering mode, but in practice this is probably meaningless.

   // Create the chain
   gl::glCreateTextures(gl::GL_TEXTURE_2D, 1, &chain->object);
   gl::glTextureParameteri(chain->object, gl::GL_TEXTURE_MAG_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(chain->object, gl::GL_TEXTURE_MIN_FILTER, static_cast<int>(gl::GL_NEAREST));
   gl::glTextureParameteri(chain->object, gl::GL_TEXTURE_WRAP_S, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glTextureParameteri(chain->object, gl::GL_TEXTURE_WRAP_T, static_cast<int>(gl::GL_CLAMP_TO_EDGE));
   gl::glTextureStorage2D(chain->object, 1, gl::GL_RGBA8, data.width, data.height);
   chain->width = data.width;
   chain->height = data.height;

   // Initialize the pixels to a more useful color
#define rf_to_ru(x) (static_cast<uint32_t>(x * 256) & 0xFF)
#define rgbaf_to_rgbau(r,g,b,a) (rf_to_ru(r) | (rf_to_ru(g) << 8) | (rf_to_ru(b) << 16) | (rf_to_ru(a) << 24))
   uint32_t pixelCount = data.width * data.height;
   uint32_t *tmpClearBuf = new uint32_t[pixelCount];
   uint32_t clearColor = rgbaf_to_rgbau(0.7f, 0.3f, 0.3f, 1.0f);
   for (uint32_t i = 0; i < pixelCount; ++i) {
      tmpClearBuf[i] = clearColor;
   }
   gl::glTextureSubImage2D(chain->object, 0, 0, 0, data.width, data.height, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, tmpClearBuf);
   delete[] tmpClearBuf;
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
   ScanBufferChain *target = nullptr;

   if (data.scanTarget == SCANTARGET_TV) {
      target = &mTvScanBuffers;
   } else if (data.scanTarget == SCANTARGET_DRC) {
      target = &mDrcScanBuffers;
   } else {
      gLog->error("decafCopyColorToScan called for unknown scanTarget.");
      return;
   }

   auto pitch_tile_max = data.cb_color_size.PITCH_TILE_MAX();
   auto slice_tile_max = data.cb_color_size.SLICE_TILE_MAX();

   auto pitch = gsl::narrow_cast<gl::GLsizei>((pitch_tile_max + 1) * latte::MicroTileWidth);
   auto height = gsl::narrow_cast<gl::GLsizei>(((slice_tile_max + 1) * (latte::MicroTileWidth * latte::MicroTileHeight)) / pitch);

   gl::glNamedFramebufferTexture(mBlitFrameBuffers[0], gl::GL_COLOR_ATTACHMENT0, target->object, 0);
   gl::glNamedFramebufferTexture(mBlitFrameBuffers[1], gl::GL_COLOR_ATTACHMENT0, buffer->object, 0);

   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glBlitNamedFramebuffer(mBlitFrameBuffers[1], mBlitFrameBuffers[0],
      0, 0, pitch, height,
      0, 0, target->width, target->height,
      gl::GL_COLOR_BUFFER_BIT, gl::GL_LINEAR);
   gl::glEnable(gl::GL_SCISSOR_TEST);
}

void GLDriver::decafSwapBuffers(const pm4::DecafSwapBuffers &data)
{
   static const auto weight = 0.9;

   // We do not need to actually call swap as our driver does this
   //  automatically with the vsync.  We do however need to make sure
   //  that we've finished all our OpenGL commands before we continue,
   //  otherwise, we may overwrite a buffer used on this frame.
   gl::glFinish();

   // TODO: We should have a render chain of 2 buffers so that we don't render stuff
   //  until the game actually asked us to.

   gx2::internal::onFlip();

   auto now = std::chrono::system_clock::now();

   if (mLastSwap.time_since_epoch().count()) {
      mAverageFrameTime = weight * mAverageFrameTime + (1.0 - weight) * (now - mLastSwap);
   }

   mLastSwap = now;

   if (mSwapFunc) {
      mSwapFunc(mTvScanBuffers.object, mDrcScanBuffers.object);
   }
}

void GLDriver::decafSetContextState(const pm4::DecafSetContextState &data)
{
   mContextState = reinterpret_cast<latte::ContextState *>(data.context.get());
}

void GLDriver::decafInvalidate(const pm4::DecafInvalidate &data)
{
   auto start = data.memStart;
   auto end = data.memEnd;

   for (auto &surf : mSurfaces) {
      if (surf.second.cpuMemStart >= end || surf.second.cpuMemEnd < start) {
         continue;
      }

      surf.second.dirtyAsTexture = true;
   }
}

void GLDriver::decafDebugMarker(const pm4::DecafDebugMarker &data)
{
   gLog->trace("GPU Debug Marker: {} {}", data.key.data(), data.id);
}

void GLDriver::getSwapBuffers(unsigned int *tv, unsigned int *drc)
{
   *tv = mTvScanBuffers.object;
   *drc = mDrcScanBuffers.object;
}

float GLDriver::getAverageFPS()
{
   // TODO: This is not thread safe...
   static const auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds{ 1 }).count();
   return static_cast<float>(second / mAverageFrameTime.count());
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

void GLDriver::poll()
{
   auto buffer = gpu::unqueueCommandBuffer();

   if (!buffer) {
      return;
   }

   // Execute command buffer
   runCommandBuffer(buffer->buffer, buffer->curSize);

   // Handle end-of-pipeline events
   handlePendingEOP();

   // Release command buffer
   gpu::retireCommandBuffer(buffer);
}

void GLDriver::syncPoll(std::function<void(gl::GLuint, gl::GLuint)> swapFunc)
{
   if (!mRunning) {
      initGL();
      mRunning = true;
   }

   mSwapFunc = swapFunc;
   poll();
}

void GLDriver::run()
{
   mRunning = true;

   initGL();

   while (mRunning) {
      poll();
   }
}

void GLDriver::stop()
{
   mRunning = false;

   // Wake the GPU thread
   gpu::queueUserBuffer(nullptr, 0);
}

} // namespace opengl

} // namespace gpu
