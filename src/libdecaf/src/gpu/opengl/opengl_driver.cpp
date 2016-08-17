#include "common/decaf_assert.h"
#include "common/log.h"
#include "gpu/commandqueue.h"
#include "gpu/latte_registers.h"
#include "gpu/pm4_buffer.h"
#include "modules/coreinit/coreinit_time.h"
#include "modules/gx2/gx2_event.h"
#include "modules/gx2/gx2_enum.h"
#include "opengl_constants.h"
#include "opengl_driver.h"
#include <fstream>
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

namespace gpu
{

namespace opengl
{

unsigned
MaxUniformBlockSize = 0;

void
GLDriver::initGL()
{
   // Clear active state
   mRegisters.fill(0);
   for (auto i = 0u; i < mRegisters.size(); ++i) {
      applyRegister(static_cast<latte::Register>(i*4));
   }
   mActiveShader = nullptr;
   mActiveDepthBuffer = nullptr;
   memset(&mActiveColorBuffers[0], 0, sizeof(SurfaceBuffer *) * mActiveColorBuffers.size());

   // Create our blit framebuffer
   gl::glCreateFramebuffers(2, mBlitFrameBuffers);

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer);

   // Create framebuffers for color-clear and depth-clear operations
   gl::glCreateFramebuffers(1, &mColorClearFrameBuffer);
   gl::glCreateFramebuffers(1, &mDepthClearFrameBuffer);

   gl::GLint value;
   gl::glGetIntegerv(gl::GL_MAX_UNIFORM_BLOCK_SIZE, &value);
   MaxUniformBlockSize = value;
}

void
GLDriver::decafSetBuffer(const pm4::DecafSetBuffer &data)
{
   auto chain = data.isTv ? &mTvScanBuffers : &mDrcScanBuffers;

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
   auto pixelCount = data.width * data.height;
   auto tmpClearBuf = new uint32_t[pixelCount];
   auto clearColor = rgbaf_to_rgbau(0.7f, 0.3f, 0.3f, 1.0f);

   for (auto i = 0u; i < pixelCount; ++i) {
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

void
GLDriver::decafCopyColorToScan(const pm4::DecafCopyColorToScan &data)
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

   gl::glNamedFramebufferTexture(mBlitFrameBuffers[0], gl::GL_COLOR_ATTACHMENT0, target->object, 0);
   gl::glNamedFramebufferTexture(mBlitFrameBuffers[1], gl::GL_COLOR_ATTACHMENT0, buffer->active->object, 0);

   gl::glDisable(gl::GL_SCISSOR_TEST);
   gl::glBlitNamedFramebuffer(mBlitFrameBuffers[1], mBlitFrameBuffers[0],
                              0, 0, data.width, data.height,
                              0, 0, target->width, target->height,
                              gl::GL_COLOR_BUFFER_BIT, gl::GL_LINEAR);
   gl::glEnable(gl::GL_SCISSOR_TEST);
}

void
GLDriver::decafSwapBuffers(const pm4::DecafSwapBuffers &data)
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

void
GLDriver::decafSetContextState(const pm4::DecafSetContextState &data)
{
   mContextState = reinterpret_cast<latte::ContextState *>(data.context.get());
}

void
GLDriver::decafDebugMarker(const pm4::DecafDebugMarker &data)
{
   gLog->trace("GPU Debug Marker: {} {}", data.key.data(), data.id);
}

void
GLDriver::decafOSScreenFlip(const pm4::DecafOSScreenFlip &data)
{
   auto texture = 0u;
   auto width = 0u;
   auto height = 0u;

   if (data.screen == 0) {
      texture = mTvScanBuffers.object;
      width = mTvScanBuffers.width;
      height = mTvScanBuffers.height;
   } else {
      texture = mDrcScanBuffers.object;
      width = mDrcScanBuffers.width;
      height = mDrcScanBuffers.height;
   }

   gl::glTextureSubImage2D(texture, 0, 0, 0, width, height, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, data.buffer);
   decafSwapBuffers(pm4::DecafSwapBuffers {});
}

// TODO: Move all these GL things into a common place!
static gl::GLenum
getTextureTarget(latte::SQ_TEX_DIM dim)
{
   switch (dim)
   {
   case latte::SQ_TEX_DIM_1D:
      return gl::GL_TEXTURE_1D;
   case latte::SQ_TEX_DIM_2D:
      return gl::GL_TEXTURE_2D;
   case latte::SQ_TEX_DIM_3D:
      return gl::GL_TEXTURE_3D;
   case latte::SQ_TEX_DIM_CUBEMAP:
      return gl::GL_TEXTURE_CUBE_MAP;
   case latte::SQ_TEX_DIM_1D_ARRAY:
      return gl::GL_TEXTURE_1D_ARRAY;
   case latte::SQ_TEX_DIM_2D_ARRAY:
      return gl::GL_TEXTURE_2D_ARRAY;
   case latte::SQ_TEX_DIM_2D_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE;
   case latte::SQ_TEX_DIM_2D_ARRAY_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
   default:
      decaf_abort(fmt::format("Unimplemented SQ_TEX_DIM {}", dim));
   }
}

void
GLDriver::decafCopySurface(const pm4::DecafCopySurface &data)
{
   decaf_check(data.dstPitch <= data.srcPitch);
   decaf_check(data.dstWidth == data.srcWidth);
   decaf_check(data.dstHeight == data.srcHeight);
   decaf_check(data.dstDepth == data.srcDepth);
   decaf_check(data.dstDim == data.srcDim);

   auto dstBuffer = getSurfaceBuffer(
      data.dstImage,
      data.dstPitch,
      data.dstWidth,
      data.dstHeight,
      data.dstDepth,
      data.dstDim,
      data.dstFormat,
      data.dstNumFormat,
      data.dstFormatComp,
      data.dstDegamma,
      false,
      data.dstTileMode,
      true);

   auto srcBuffer = getSurfaceBuffer(
      data.srcImage,
      data.srcPitch,
      data.srcWidth,
      data.srcHeight,
      data.srcDepth,
      data.srcDim,
      data.srcFormat,
      data.srcNumFormat,
      data.srcFormatComp,
      data.srcDegamma,
      false,
      data.srcTileMode,
      false);

   gl::glCopyImageSubData(
      srcBuffer->active->object,
      getTextureTarget(data.dstDim),
      data.srcLevel,
      0, 0, data.srcSlice,
      dstBuffer->active->object,
      getTextureTarget(data.srcDim),
      data.dstLevel,
      0, 0, data.dstSlice,
      data.dstWidth,
      data.dstHeight,
      data.dstDepth);

   dstBuffer->dirtyAsTexture = false;
}

void
GLDriver::surfaceSync(const pm4::SurfaceSync &data)
{
   auto memStart = data.addr << 8;
   auto memEnd = memStart + (data.size << 8);

   auto all = data.cp_coher_cntl.FULL_CACHE_ENA();
   auto surfaces = all;
   auto shader = all;
   auto shaderExport = all;

   if (data.cp_coher_cntl.TC_ACTION_ENA()) {
      surfaces = true;
   }

   if (data.cp_coher_cntl.CB_ACTION_ENA()) {
      surfaces = true;
   }

   if (data.cp_coher_cntl.DB_ACTION_ENA()) {
      surfaces = true;
   }

   if (data.cp_coher_cntl.SH_ACTION_ENA()) {
      shader = true;
   }

   if (data.cp_coher_cntl.SX_ACTION_ENA()) {
      shaderExport = true;
   }

   if (surfaces) {
      for (auto &surf : mSurfaces) {
         if (surf.second.cpuMemStart >= memEnd || surf.second.cpuMemEnd < memStart) {
            continue;
         }

         surf.second.dirtyAsTexture = true;
      }
   }

   for (auto &buffer : mDataBuffers) {
      DataBuffer *dataBuffer = &buffer.second;

      if (dataBuffer->cpuMemStart >= memEnd || dataBuffer->cpuMemEnd < memStart) {
         continue;
      }

      auto offset = std::max(memStart, dataBuffer->cpuMemStart) - dataBuffer->cpuMemStart;
      auto size = (std::min(memEnd, dataBuffer->cpuMemEnd) - dataBuffer->cpuMemStart) - offset;

      if (dataBuffer->isOutput && shaderExport) {
         downloadDataBuffer(dataBuffer, offset, size);
      } else if (dataBuffer->isInput && (shader || surfaces)) {
         uploadDataBuffer(dataBuffer, offset, size);
      }
   }
}

void
GLDriver::getSwapBuffers(unsigned int *tv,
                         unsigned int *drc)
{
   *tv = mTvScanBuffers.object;
   *drc = mDrcScanBuffers.object;
}

float
GLDriver::getAverageFPS()
{
   // TODO: This is not thread safe...
   static const auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds{ 1 }).count();
   return static_cast<float>(second / mAverageFrameTime.count());
}

uint64_t
GLDriver::getGpuClock()
{
   return coreinit::OSGetTime();
}

void
GLDriver::memWrite(const pm4::MemWrite &data)
{
   auto value = uint64_t { 0 };
   auto addr = mem::translate(data.addrLo.ADDR_LO() << 2);

   if (data.addrHi.CNTR_SEL() == pm4::MW_WRITE_CLOCK) {
      value = getGpuClock();
   } else {
      value = static_cast<uint64_t>(data.dataLo) | static_cast<uint64_t>(data.dataHi) << 32;
   }

   switch (data.addrLo.ENDIAN_SWAP())
   {
   case latte::CB_ENDIAN_NONE:
      break;
   case latte::CB_ENDIAN_8IN64:
      value = byte_swap(value);
      break;
   case latte::CB_ENDIAN_8IN32:
      value = byte_swap(static_cast<uint32_t>(value));
      break;
   case latte::CB_ENDIAN_8IN16:
      decaf_abort(fmt::format("Unexpected MEM_WRITE endian swap {}", data.addrLo.ENDIAN_SWAP()));
   }

   if (data.addrHi.DATA32()) {
      *reinterpret_cast<uint32_t *>(addr) = static_cast<uint32_t>(value);
   } else {
      *reinterpret_cast<uint64_t *>(addr) = value;
   }
}

void
GLDriver::eventWrite(const pm4::EventWrite &data)
{
   auto type = data.eventInitiator.EVENT_TYPE();
   auto addr = data.addrLo.ADDR_LO() << 2;
   auto ptr = mem::translate(addr);
   auto value = uint64_t { 0 };

   decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

   switch (type) {
   case latte::VGT_EVENT_TYPE_ZPASS_DONE:
      // This seems to be an instantaneous counter fetch, but OpenGL doesn't
      //  expose the raw counter, so we detect GX2QueryBegin/End by the
      //  write address and translate this to a SAMPLES_PASSED query.
      if (addr == mLastOccQueryAddress + 8) {
         mLastOccQueryAddress = 0;

         decaf_check(mOccQuery);
         gl::glEndQuery(gl::GL_SAMPLES_PASSED);
         gl::glGetQueryObjectui64v(mOccQuery, gl::GL_QUERY_RESULT, &value);
      } else {
         decaf_check(!mLastOccQueryAddress);
         mLastOccQueryAddress = addr;

         if (!mOccQuery) {
            gl::glGenQueries(1, &mOccQuery);
            decaf_check(mOccQuery);
         }

         gl::glBeginQuery(gl::GL_SAMPLES_PASSED, mOccQuery);
      }
      break;
   default:
      decaf_abort(fmt::format("Unexpected event type {}", type));
   }

   switch (data.addrLo.ENDIAN_SWAP())
   {
   case latte::CB_ENDIAN_NONE:
      break;
   case latte::CB_ENDIAN_8IN64:
      value = byte_swap(value);
      break;
   case latte::CB_ENDIAN_8IN32:
      value = byte_swap(static_cast<uint32_t>(value));
      break;
   case latte::CB_ENDIAN_8IN16:
      decaf_abort("Unexpected EVENT_WRITE endian swap 8IN16");
   }

   switch (data.addrHi.DATA_SEL()) {
   case pm4::EW_DATA_DISCARD:
      break;
   case pm4::EW_DATA_32:
      *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
      break;
   case pm4::EW_DATA_64:
      *reinterpret_cast<uint64_t *>(ptr) = value;
      break;
   case pm4::EW_DATA_CLOCK:
      decaf_abort("Unexpected EW_DATA_CLOCK in EVENT_WRITE");
   }
}

void
GLDriver::eventWriteEOP(const pm4::EventWriteEOP &data)
{
   mPendingEOP = data;
}

void
GLDriver::handlePendingEOP()
{
   if (!mPendingEOP.eventInitiator.EVENT_TYPE()) {
      return;
   }

   auto value = uint64_t { 0 };
   auto addr = mPendingEOP.addrLo.ADDR_LO() << 2;
   auto ptr = mem::translate(addr);

   decaf_assert(mPendingEOP.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

   switch (mPendingEOP.eventInitiator.EVENT_TYPE()) {
   case latte::VGT_EVENT_TYPE_BOTTOM_OF_PIPE_TS:
      value = getGpuClock();
      break;
   default:
      decaf_abort(fmt::format("Unexpected EOP event type {}", mPendingEOP.eventInitiator.EVENT_TYPE()));
   }

   switch (mPendingEOP.addrLo.ENDIAN_SWAP()) {
   case latte::CB_ENDIAN_NONE:
      break;
   case latte::CB_ENDIAN_8IN64:
      value = byte_swap(value);
      break;
   case latte::CB_ENDIAN_8IN32:
      value = byte_swap(static_cast<uint32_t>(value));
      break;
   case latte::CB_ENDIAN_8IN16:
      decaf_abort(fmt::format("Unexpected EOP event endian swap {}", mPendingEOP.addrLo.ENDIAN_SWAP()));
   }

   switch (mPendingEOP.addrHi.DATA_SEL()) {
   case pm4::EW_DATA_DISCARD:
      break;
   case pm4::EW_DATA_32:
      *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
      break;
   case pm4::EW_DATA_64:
   case pm4::EW_DATA_CLOCK:
      *reinterpret_cast<uint64_t *>(ptr) = value;
      break;
   }

   std::memset(&mPendingEOP, 0, sizeof(pm4::EventWriteEOP));
}

void
GLDriver::pfpSyncMe(const pm4::PfpSyncMe &data)
{
   // TODO: do we need to do anything?
}

void
GLDriver::executeBuffer(pm4::Buffer *buffer)
{
   // Execute command buffer
   runCommandBuffer(buffer->buffer, buffer->curSize);

   // Handle end-of-pipeline events
   handlePendingEOP();

   // Release command buffer
   gpu::retireCommandBuffer(buffer);
}

void
GLDriver::syncPoll(const SwapFunction &swapFunc)
{
   if (mRunState == RunState::None) {
      initGL();
      mRunState = RunState::Running;
   }

   mSwapFunc = swapFunc;

   if (auto buffer = gpu::tryUnqueueCommandBuffer()) {
      executeBuffer(buffer);
   }
}

void
GLDriver::run()
{
   if (mRunState != RunState::None) {
      return;
   }

   mRunState = RunState::Running;
   initGL();

   while (mRunState == RunState::Running) {
      auto buffer = gpu::unqueueCommandBuffer();

      if (buffer) {
         executeBuffer(buffer);
      }
   }
}

void
GLDriver::stop()
{
   mRunState = RunState::Stopped;

   // Wake the GPU thread
   gpu::awaken();
}

} // namespace opengl

} // namespace gpu
