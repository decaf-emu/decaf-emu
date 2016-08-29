#include "common/decaf_assert.h"
#include "common/log.h"
#include "decaf_config.h"
#include "gpu/gpu_commandqueue.h"
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
   mDrawBuffers.fill(gl::GL_NONE);
   mGLStateCache.blendEnable.fill(false);
   mLastUniformUpdate.fill(0);

   // We always use the scissor test
   gl::glEnable(gl::GL_SCISSOR_TEST);

   // We always use GL_UPPER_LEFT coordinates
   gl::glClipControl(gl::GL_UPPER_LEFT, gl::GL_NEGATIVE_ONE_TO_ONE);

   // The GL spec doesn't say whether these default to enabled or disabled.
   //  They're probably disabled, but let's play it safe.
   gl::glDisable(gl::GL_DEPTH_CLAMP);
   gl::glDisable(gl::GL_PRIMITIVE_RESTART);

   // Create our blit framebuffer
   gl::glCreateFramebuffers(2, mBlitFrameBuffers);

   if (decaf::config::gpu::debug) {
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mBlitFrameBuffers[0], -1, "blit target");
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mBlitFrameBuffers[1], -1, "blit source");
   }

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer);

   // Create framebuffers for color-clear and depth-clear operations
   gl::glCreateFramebuffers(1, &mColorClearFrameBuffer);
   gl::glCreateFramebuffers(1, &mDepthClearFrameBuffer);

   if (decaf::config::gpu::debug) {
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mColorClearFrameBuffer, -1, "color clear");
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mDepthClearFrameBuffer, -1, "depth clear");
   }

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

   if (decaf::config::gpu::debug) {
      const char *label = data.isTv ? "TV framebuffer" : "DRC framebuffer";
      gl::glObjectLabel(gl::GL_TEXTURE, chain->object, -1, label);
   }

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
   auto buffer = getColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info, false);
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

   injectFence([=]() {
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
   });
}

void
GLDriver::decafSetSwapInterval(const pm4::DecafSetSwapInterval &data)
{
   decaf_assert(data.interval <= 10, fmt::format("Bizarre swap interval {}", data.interval));
   mSwapInterval = data.interval;
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
      data.dstSamples,
      data.dstDim,
      data.dstFormat,
      data.dstNumFormat,
      data.dstFormatComp,
      data.dstDegamma,
      false,
      data.dstTileMode,
      true,
      true);

   auto srcBuffer = getSurfaceBuffer(
      data.srcImage,
      data.srcPitch,
      data.srcWidth,
      data.srcHeight,
      data.srcDepth,
      data.srcSamples,
      data.srcDim,
      data.srcFormat,
      data.srcNumFormat,
      data.srcFormatComp,
      data.srcDegamma,
      false,
      data.srcTileMode,
      false,
      false);

   auto copyWidth = data.srcWidth;
   auto copyHeight = data.srcHeight;
   auto copyDepth = data.srcDepth;
   if (data.srcDim == latte::SQ_TEX_DIM_CUBEMAP) {
      copyDepth *= 6;
   }

   gl::glCopyImageSubData(
      srcBuffer->active->object,
      getTextureTarget(data.dstDim),
      data.srcLevel,
      0, 0, data.srcSlice,
      dstBuffer->active->object,
      getTextureTarget(data.srcDim),
      data.dstLevel,
      0, 0, data.dstSlice,
      copyWidth,
      copyHeight,
      copyDepth);

   dstBuffer->needUpload = false;
}

void
GLDriver::surfaceSync(const pm4::SurfaceSync &data)
{
   std::unique_lock<std::mutex> lock(mResourceMutex);

   auto memStart = data.addr << 8;
   auto memEnd = memStart + (data.size << 8);

   auto all = data.cp_coher_cntl.FULL_CACHE_ENA();
   auto surfaces = all;
   auto shaders = all;
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
      shaders = true;
   }

   if (data.cp_coher_cntl.SX_ACTION_ENA()) {
      shaderExport = true;
   }

   if (surfaces) {
      for (auto &i : mSurfaces) {
         SurfaceBuffer *surface = &i.second;

         if (surface->cpuMemStart < memEnd && surface->cpuMemEnd > memStart) {
            surface->needUpload |= surface->dirtyMemory;
            surface->dirtyMemory = false;
         }
      }
   }

   if (shaders) {
      for (auto &i : mFetchShaders) {
         Shader *shader = i.second;

         if (shader->cpuMemStart < memEnd && shader->cpuMemEnd > memStart) {
            shader->needRebuild |= shader->dirtyMemory;
            shader->dirtyMemory = false;
         }
      }

      for (auto &i : mVertexShaders) {
         Shader *shader = i.second;

         if (shader->cpuMemStart < memEnd && shader->cpuMemEnd > memStart) {
            shader->needRebuild |= shader->dirtyMemory;
            shader->dirtyMemory = false;
         }
      }

      for (auto &i : mPixelShaders) {
         Shader *shader = i.second;

         if (shader->cpuMemStart < memEnd && shader->cpuMemEnd > memStart) {
            shader->needRebuild |= shader->dirtyMemory;
            shader->dirtyMemory = false;
         }
      }
   }

   for (auto &i : mDataBuffers) {
      DataBuffer *buffer = &i.second;

      if (buffer->cpuMemStart < memEnd && buffer->cpuMemEnd > memStart) {
         auto offset = std::max(memStart, buffer->cpuMemStart) - buffer->cpuMemStart;
         auto size = (std::min(memEnd, buffer->cpuMemEnd) - buffer->cpuMemStart) - offset;

         if (buffer->isInput && buffer->dirtyMemory && (shaders || surfaces)) {
            uploadDataBuffer(buffer, offset, size);
            buffer->dirtyMemory = false;
         }
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
   injectFence([=]() {
      auto value = uint64_t{ 0 };
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
   });
}

void
GLDriver::eventWrite(const pm4::EventWrite &data)
{
   auto type = data.eventInitiator.EVENT_TYPE();
   auto addr = data.addrLo.ADDR_LO() << 2;
   auto ptr = mem::translate(addr);

   decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

   auto writeData = [=](uint64_t value) {
      switch (data.addrLo.ENDIAN_SWAP()) {
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

      *reinterpret_cast<uint64_t *>(ptr) = value;
   };

   switch (type) {
   case latte::VGT_EVENT_TYPE_ZPASS_DONE: {
      if (!mOccQuery) {
         injectFence([=]() {
            writeData(mTotalSamplesPassed);
         });
      } else {
         gl::glEndQuery(gl::GL_SAMPLES_PASSED);

         auto originalQuery = mOccQuery;

         SyncWait wait;
         wait.type = SyncWaitType::Query;
         wait.query = originalQuery;
         wait.func = [=]() {
            auto value = uint64_t{ 0 };
            gl::glGetQueryObjectui64v(originalQuery, gl::GL_QUERY_RESULT, &value);
            mTotalSamplesPassed += value;
            writeData(mTotalSamplesPassed);
         };
         mSyncWaits.emplace(wait);
      }

      gl::glGenQueries(1, &mOccQuery);
      gl::glBeginQuery(gl::GL_SAMPLES_PASSED, mOccQuery);
   } break;
   default:
      decaf_abort(fmt::format("Unexpected event type {}", type));
   }
}

void
GLDriver::eventWriteEOP(const pm4::EventWriteEOP &data)
{
   if (!data.eventInitiator.EVENT_TYPE()) {
      return;
   }

   injectFence([=]() {
      auto value = uint64_t{ 0 };
      auto addr = data.addrLo.ADDR_LO() << 2;
      auto ptr = mem::translate(addr);

      decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

      switch (data.eventInitiator.EVENT_TYPE()) {
      case latte::VGT_EVENT_TYPE_BOTTOM_OF_PIPE_TS:
         value = getGpuClock();
         break;
      default:
         decaf_abort(fmt::format("Unexpected EOP event type {}", data.eventInitiator.EVENT_TYPE()));
      }

      switch (data.addrLo.ENDIAN_SWAP()) {
      case latte::CB_ENDIAN_NONE:
         break;
      case latte::CB_ENDIAN_8IN64:
         value = byte_swap(value);
         break;
      case latte::CB_ENDIAN_8IN32:
         value = byte_swap(static_cast<uint32_t>(value));
         break;
      case latte::CB_ENDIAN_8IN16:
         decaf_abort("Unexpected EVENT_WRITE_EOP endian swap 8IN16");
      }

      switch (data.addrHi.DATA_SEL()) {
      case pm4::EWP_DATA_DISCARD:
         break;
      case pm4::EWP_DATA_32:
         *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
         break;
      case pm4::EWP_DATA_64:
      case pm4::EWP_DATA_CLOCK:
         *reinterpret_cast<uint64_t *>(ptr) = value;
         break;
      }
   });
}

void
GLDriver::pfpSyncMe(const pm4::PfpSyncMe &data)
{
   // TODO: do we need to do anything?
}

void
GLDriver::injectFence(std::function<void()> func)
{
   gl::GLsync object = gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, gl::GL_NONE_BIT);

   SyncWait wait;
   wait.type = SyncWaitType::Fence;
   wait.fence = object;
   wait.func = func;
   mSyncWaits.emplace(wait);
}

void
GLDriver::checkSyncObjects()
{
   while (true) {
      if (!mSyncWaits.size()) {
         break;
      }

      auto &wait = mSyncWaits.front();

      if (wait.type == SyncWaitType::Fence) {
         gl::GLenum value;
         gl::glGetSynciv(wait.fence, gl::GL_SYNC_STATUS, 4, nullptr, reinterpret_cast<gl::GLint*>(&value));

         if (value == gl::GL_UNSIGNALED) {
            break;
         }

         wait.func();
         glDeleteSync(wait.fence);
      } else if (wait.type == SyncWaitType::Query) {
         gl::GLboolean value;
         gl::glGetQueryObjectuiv(wait.query, gl::GL_QUERY_RESULT_AVAILABLE, reinterpret_cast<gl::GLuint*>(&value));

         if (value == gl::GL_FALSE) {
            break;
         }

         wait.func();
         gl::glDeleteQueries(1, &wait.query);
      } else {
         decaf_abort("GPU thread encountered unknown sync type");
      }

      mSyncWaits.pop();
   }
}

void
GLDriver::executeBuffer(pm4::Buffer *buffer)
{
   // Execute command buffer
   runCommandBuffer(buffer->buffer, buffer->curSize);

   // Release command buffer
   injectFence([=]() {
      gpu::retireCommandBuffer(buffer);
   });

   // Flush the OpenGL command stream
   gl::glFlush();
}

void
GLDriver::runOnGLThread(std::function<void()> func)
{
   std::unique_lock<std::mutex> lock(mTaskListMutex);

   mTaskList.emplace_back(func);
   auto taskIterator = mTaskList.end();
   --taskIterator;

   // Submit a dummy command buffer in case the GPU is idle
   // (TODO: use a separate wakeup signal shared between command buffers
   //  and tasks instead of waiting on the command buffer queue directly)
   static const uint32_t nopPacket = byte_swap(
      pm4::type2::Header::get(0).type(pm4::Header::Type2).value);
   pm4::Buffer nopBuffer;
   nopBuffer.buffer = const_cast<uint32_t *>(&nopPacket);
   nopBuffer.curSize = sizeof(nopPacket);
   nopBuffer.maxSize = sizeof(nopPacket);
   gpu::queueCommandBuffer(&nopBuffer);

   taskIterator->completionCV.wait(lock);

   mTaskList.erase(taskIterator);
}

void
GLDriver::runRemoteThreadTasks()
{
   std::unique_lock<std::mutex> lock(mTaskListMutex);

   for (auto &i : mTaskList) {
      i.func();
      i.completionCV.notify_all();
   }
}

void
GLDriver::notifyCpuFlush(void *ptr,
                         uint32_t size)
{
   std::unique_lock<std::mutex> lock(mResourceMutex);
   auto memStart = mem::untranslate(ptr);
   auto memEnd = memStart + size;

   for (auto &i : mFetchShaders) {
      auto resource = i.second;

      if (resource && resource->cpuMemStart < memEnd && resource->cpuMemEnd > memStart) {
         resource->dirtyMemory = true;
      }
   }

   for (auto &i : mVertexShaders) {
      auto resource = i.second;

      if (resource && resource->cpuMemStart < memEnd && resource->cpuMemEnd > memStart) {
         resource->dirtyMemory = true;
      }
   }

   for (auto &i : mPixelShaders) {
      auto resource = i.second;

      if (resource && resource->cpuMemStart < memEnd && resource->cpuMemEnd > memStart) {
         resource->dirtyMemory = true;
      }
   }

   for (auto &i : mSurfaces) {
      auto resource = &i.second;

      if (resource->cpuMemStart < memEnd && resource->cpuMemEnd > memStart) {
         resource->dirtyMemory = true;
      }
   }

   for (auto &i : mDataBuffers) {
      auto resource = &i.second;

      if (resource->cpuMemStart < memEnd && resource->cpuMemEnd > memStart) {
         resource->dirtyMemory = true;
      }
   }
}

void
GLDriver::notifyGpuFlush(void *ptr,
                         uint32_t size)
{
   std::unique_lock<std::mutex> lock(mResourceMutex);

   auto memStart = mem::untranslate(ptr);
   auto memEnd = memStart + size;

   for (auto &i : mDataBuffers) {
      DataBuffer *buffer = &i.second;
      if (buffer->isOutput && buffer->cpuMemStart < memEnd && buffer->cpuMemEnd > memStart) {
         auto offset = std::max(memStart, buffer->cpuMemStart) - buffer->cpuMemStart;
         auto size = (std::min(memEnd, buffer->cpuMemEnd) - buffer->cpuMemStart) - offset;

         runOnGLThread([=](){
            downloadDataBuffer(buffer, offset, size);
         });

         buffer->dirtyMemory = false;
      }
   }
}

void
GLDriver::syncPoll(const SwapFunction &swapFunc)
{
   if (mRunState == RunState::None) {
      initGL();
      mRunState = RunState::Running;
   }

   mSwapFunc = swapFunc;

   while (auto buffer = gpu::tryUnqueueCommandBuffer()) {
      executeBuffer(buffer);
      checkSyncObjects();
   }

   checkSyncObjects();
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
      pm4::Buffer *buffer;

      if (mSyncWaits.size() == 0) {
         buffer = gpu::unqueueCommandBuffer();
      } else {
         buffer = gpu::tryUnqueueCommandBuffer();
      }

      if (buffer) {
         executeBuffer(buffer);
      } else {
         checkSyncObjects();
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
