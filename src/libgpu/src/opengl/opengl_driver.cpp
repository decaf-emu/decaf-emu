#ifdef DECAF_GL
#include "gpu_clock.h"
#include "gpu_config.h"
#include "gpu_configstorage.h"
#include "gpu_event.h"
#include "gpu_ih.h"
#include "gpu_ringbuffer.h"
#include "gpu_memory.h"
#include "latte/latte_endian.h"
#include "latte/latte_registers.h"
#include "opengl_constants.h"
#include "opengl_driver.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <common/tga_encoder.h>
#include <fmt/format.h>
#include <fstream>
#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
#include <gsl.h>

namespace opengl
{

GLDriver::GLDriver()
{
   mRegisters.fill(0);
}

gpu::GraphicsDriverType
GLDriver::type()
{
   return gpu::GraphicsDriverType::OpenGL;
}

void
GLDriver::initialise()
{
   // Register config change handler
   static std::once_flag sRegisteredConfigChangeListener;
   std::call_once(sRegisteredConfigChangeListener,
      [this]() {
         gpu::registerConfigChangeListener(
            [this](const gpu::Settings &settings) {
               mDebug = settings.debug.debug_enabled;
               mDumpShaders = settings.debug.dump_shaders;
            });
      });

   // Read config
   auto gpuConfig = gpu::config();
   mDebug = gpuConfig->debug.debug_enabled;
   mDumpShaders = gpuConfig->debug.dump_shaders;

   // Apply all registers
   for (auto i = 0u; i < mRegisters.size(); ++i) {
      if (i * 4 == latte::Register::VGT_STRMOUT_DRAW_OPAQUE_BUFFER_FILLED_SIZE) {
         continue;
      }
      applyRegister(static_cast<latte::Register>(i * 4));
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

   if (mDebug) {
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mBlitFrameBuffers[0], -1, "blit target");
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mBlitFrameBuffers[1], -1, "blit source");
   }

   // Create our default framebuffer
   gl::glGenFramebuffers(1, &mFrameBuffer);
   gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, mFrameBuffer);

   // Create framebuffers for color-clear and depth-clear operations
   gl::glCreateFramebuffers(1, &mColorClearFrameBuffer);
   gl::glCreateFramebuffers(1, &mDepthClearFrameBuffer);

   if (mDebug) {
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mColorClearFrameBuffer, -1, "color clear");
      gl::glObjectLabel(gl::GL_FRAMEBUFFER, mDepthClearFrameBuffer, -1, "depth clear");
   }

   gl::GLint value;
   gl::glGetIntegerv(gl::GL_MAX_UNIFORM_BLOCK_SIZE, &value);
   MaxUniformBlockSize = value;
}

void
GLDriver::shutdown()
{
}

void
GLDriver::decafSetBuffer(const latte::pm4::DecafSetBuffer &data)
{
   ScanBufferChain *chain;
   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      chain = &mTvScanBuffers;
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
      chain = &mDrcScanBuffers;
   } else {
      decaf_abort("Unexpected decafSetBuffer target");
   }

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

   if (mDebug) {
      if (data.scanTarget == latte::pm4::ScanTarget::TV) {
         gl::glObjectLabel(gl::GL_TEXTURE, chain->object, -1, "TV framebuffer");
      } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
         gl::glObjectLabel(gl::GL_TEXTURE, chain->object, -1, "DRC framebuffer");
      }
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

void
GLDriver::decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data)
{
   auto buffer = getColorBuffer(data.cb_color_base, data.cb_color_size, data.cb_color_info, false);
   ScanBufferChain *target = nullptr;

   if (data.scanTarget == latte::pm4::ScanTarget::TV) {
      target = &mTvScanBuffers;
   } else if (data.scanTarget == latte::pm4::ScanTarget::DRC) {
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

bool
GLDriver::startFrameCapture(const std::string &outPrefix,
                            bool captureTV,
                            bool captureDRC)
{
   mFrameCapturePrefix = outPrefix;
   mFrameCaptureTV = captureTV;
   mFrameCaptureDRC = captureDRC;
   return true;
}

size_t
GLDriver::stopFrameCapture()
{
   mFrameCapturePrefix.clear();
   mFrameCaptureTV = false;
   mFrameCaptureDRC = false;
   return mFramesCaptured;
}


bool
GLDriver::dumpScanBuffer(const std::string &filename, const ScanBufferChain &buf)
{
   std::vector<uint8_t> buffer;
   buffer.resize(buf.width * buf.height * 4);
   gl::glGetTextureImage(buf.object, 0, gl::GL_BGRA, gl::GL_UNSIGNED_BYTE,
                         static_cast<gl::GLsizei>(buffer.size()), buffer.data());
   return tga::writeFile(filename, 32, 8, buf.width, buf.height, buffer.data());
}

void
GLDriver::decafSwapBuffers(const latte::pm4::DecafSwapBuffers &data)
{
   static const auto weight = 0.9;

   // We do not need to actually call swap as our driver does this
   //  automatically with the vsync.  We do however need to make sure
   //  that we've finished all our OpenGL commands before we continue,
   //  otherwise, we may overwrite a buffer used on this frame.
   gl::glFinish();

   // TODO: We should have a render chain of 2 buffers so that we don't render stuff
   //  until the game actually asked us to.
   gpu::onFlip();

   auto now = std::chrono::system_clock::now();

   if (mLastSwap.time_since_epoch().count()) {
      mAverageFrameTime = weight * mAverageFrameTime + (1.0 - weight) * (now - mLastSwap);
   }

   mLastSwap = now;

   if (mFrameCapturePrefix.size()) {
      if (mFrameCaptureTV) {
         dumpScanBuffer(fmt::format("{}_tv_{}.tga", mFrameCapturePrefix, mFramesCaptured), mTvScanBuffers);
      }

      if (mFrameCaptureDRC) {
         dumpScanBuffer(fmt::format("{}_drc_{}.tga", mFrameCapturePrefix, mFramesCaptured), mDrcScanBuffers);
      }

      mFramesCaptured++;
   }

   updateDebuggerInfo();
}

void
GLDriver::decafCapSyncRegisters(const latte::pm4::DecafCapSyncRegisters &data)
{
   gpu::onSyncRegisters(mRegisters.data(), static_cast<uint32_t>(mRegisters.size()));
}

void
GLDriver::decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data)
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

   gl::glTextureSubImage2D(texture, 0, 0, 0, width, height, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE,
                           gpu::internal::translateAddress(data.buffer));
   decafSwapBuffers(latte::pm4::DecafSwapBuffers {});
}

// TODO: Move all these GL things into a common place!
static gl::GLenum
getTextureTarget(latte::SQ_TEX_DIM dim)
{
   switch (dim)
   {
   case latte::SQ_TEX_DIM::DIM_1D:
      return gl::GL_TEXTURE_1D;
   case latte::SQ_TEX_DIM::DIM_2D:
      return gl::GL_TEXTURE_2D;
   case latte::SQ_TEX_DIM::DIM_3D:
      return gl::GL_TEXTURE_3D;
   case latte::SQ_TEX_DIM::DIM_CUBEMAP:
      return gl::GL_TEXTURE_CUBE_MAP;
   case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
      return gl::GL_TEXTURE_1D_ARRAY;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      return gl::GL_TEXTURE_2D_ARRAY;
   case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE;
   case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      return gl::GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
   default:
      decaf_abort(fmt::format("Unimplemented SQ_TEX_DIM {}", dim));
   }
}

void
GLDriver::decafCopySurface(const latte::pm4::DecafCopySurface &data)
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
      data.dstForceDegamma,
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
      data.srcForceDegamma,
      false,
      data.srcTileMode,
      false,
      false);

   auto copyWidth = data.srcWidth;
   auto copyHeight = data.srcHeight;
   auto copyDepth = data.srcDepth;
   if (data.srcDim == latte::SQ_TEX_DIM::DIM_CUBEMAP) {
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
GLDriver::decafExpandColorBuffer(const latte::pm4::DecafExpandColorBuffer &data)
{
   // TODO: Implement MSAA color buffers.
}

void
GLDriver::surfaceSync(const latte::pm4::SurfaceSync &data)
{
   auto memStart = phys_addr { data.addr << 8 };
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

   std::unique_lock<std::mutex> lock(mResourceMap.getMutex());
   auto iter = mResourceMap.getIterator(memStart, memEnd - memStart);

   Resource *resource;
   while ((resource = iter.next()) != nullptr) {
      switch (resource->type) {

      case Resource::SURFACE:
         if (surfaces) {
            auto surface = reinterpret_cast<SurfaceBuffer *>(resource);
            surface->needUpload |= surface->dirtyMemory;
            surface->dirtyMemory = false;
         }
         break;

      case Resource::SHADER:
         if (shaders) {
            auto shader = reinterpret_cast<Shader *>(resource);
            shader->needRebuild |= shader->dirtyMemory;
            shader->dirtyMemory = false;
         }
         break;

      case Resource::DATA_BUFFER:
         if (shaders || surfaces) {
            auto buffer = reinterpret_cast<DataBuffer *>(resource);
            if (buffer->isInput && buffer->dirtyMemory) {
               auto offset = std::max(memStart, buffer->cpuMemStart) - buffer->cpuMemStart;
               auto size = (std::min(memEnd, buffer->cpuMemEnd) - buffer->cpuMemStart) - offset;
               uploadDataBuffer(buffer, offset, size);
               buffer->dirtyMemory = false;
            }
         }
      }
   }
}

void
GLDriver::getSwapBuffers(gl::GLuint *tv,
                         gl::GLuint *drc)
{
   *tv = mTvScanBuffers.object;
   *drc = mDrcScanBuffers.object;
}

float
GLDriver::getAverageFPS()
{
   // TODO: This is not thread safe...
   static const auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds{ 1 }).count();
   auto avgFrameTime = mAverageFrameTime.count();

   if (avgFrameTime == 0.0) {
      return 0.0f;
   } else {
      return static_cast<float>(second / avgFrameTime);
   }
}

float
GLDriver::getAverageFrametimeMS()
{
   return static_cast<float>(std::chrono::duration_cast<duration_ms>(mAverageFrameTime).count());
}

gpu::OpenGLDriver::DebuggerInfo *
GLDriver::getDebuggerInfo() {
   return &mDebuggerInfo;
}

void
GLDriver::updateDebuggerInfo()
{
   mDebuggerInfo.numFetchShaders = mFetchShaders.size();
   mDebuggerInfo.numVertexShaders = mVertexShaders.size();
   mDebuggerInfo.numPixelShaders = mPixelShaders.size();
   mDebuggerInfo.numShaderPipelines = mShaderPipelines.size();
   mDebuggerInfo.numSurfaces = mSurfaces.size();
   mDebuggerInfo.numDataBuffers = mDataBuffers.size();
}

void
GLDriver::memWrite(const latte::pm4::MemWrite &data)
{
   auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
   auto ptr = gpu::internal::translateAddress(addr);
   auto value = uint64_t { 0 };

   // Read value
   if (data.addrHi.CNTR_SEL() == latte::pm4::MW_WRITE_CLOCK) {
      value = gpu::clock::now();
   } else if (data.addrHi.DATA32()) {
      value = static_cast<uint64_t>(data.dataLo);
   } else {
      value = static_cast<uint64_t>(data.dataLo) |
         (static_cast<uint64_t>(data.dataHi) << 32);
   }

   // Swap value
   value = latte::applyEndianSwap(value, data.addrLo.ENDIAN_SWAP());

   // Write value
   if (data.addrHi.DATA32()) {
      *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
   } else {
      *reinterpret_cast<uint64_t *>(ptr) = value;
   }
}

void
GLDriver::eventWrite(const latte::pm4::EventWrite &data)
{
   auto type = data.eventInitiator.EVENT_TYPE();
   auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
   auto ptr = gpu::internal::translateAddress(addr);
   auto value = uint64_t { 0 };

   decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

   switch (type) {
   case latte::VGT_EVENT_TYPE::ZPASS_DONE:
      // This seems to be an instantaneous counter fetch, but OpenGL doesn't
      //  expose the raw counter, so we detect GX2QueryBegin/End by the
      //  write address and translate this to a SAMPLES_PASSED query.
      if (addr == mLastOccQueryAddress + 8) {
         mLastOccQueryAddress = phys_addr { 0 };

         decaf_check(mOccQuery);
         gl::glEndQuery(gl::GL_SAMPLES_PASSED);

         // Perform the write when it completes instead of blocking here.
         addQuerySync(mOccQuery, [=](){
            uint64_t result;
            gl::glGetQueryObjectui64v(mOccQuery, gl::GL_QUERY_RESULT, &result);
            result = latte::applyEndianSwap(result, data.addrLo.ENDIAN_SWAP());
            *reinterpret_cast<uint64_t *>(ptr) = result;
         });
      } else {
         if (mLastOccQueryAddress) {
            gLog->warn("Program started a new occlusion query (at 0x{:X}) while one was already in progress (at 0x{:X})", mLastOccQueryAddress, addr);
            gl::glEndQuery(gl::GL_SAMPLES_PASSED);
         }
         mLastOccQueryAddress = addr;

         if (!mOccQuery) {
            gl::glGenQueries(1, &mOccQuery);
            decaf_check(mOccQuery);
         } else {
            // Ensure that any pending query has been written so we don't
            //  clobber the result.
            gl::GLint isReady;
            while (isReady = static_cast<gl::GLint>(gl::GL_TRUE),
                   gl::glGetQueryObjectiv(mOccQuery, gl::GL_QUERY_RESULT_AVAILABLE, &isReady),
                   !isReady) {
               std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
            checkSyncObjects(0);
         }

         gl::glBeginQuery(gl::GL_SAMPLES_PASSED, mOccQuery);
      }
      break;
   default:
      decaf_abort(fmt::format("Unexpected event type {}", type));
   }

   value = latte::applyEndianSwap(value, data.addrLo.ENDIAN_SWAP());
   *reinterpret_cast<uint64_t *>(ptr) = value;
}

void
GLDriver::eventWriteEOP(const latte::pm4::EventWriteEOP &data)
{
   // Write event data to memory if required
   if (data.addrHi.DATA_SEL() != latte::pm4::EWP_DATA_DISCARD) {
      auto addr = phys_addr { data.addrLo.ADDR_LO() << 2 };
      auto ptr = gpu::internal::translateAddress(addr);
      decaf_assert(data.addrHi.ADDR_HI() == 0, "Invalid event write address (high word not zero)");

      // Read value
      auto value = uint64_t { 0u };
      switch (data.addrHi.DATA_SEL()) {
      case latte::pm4::EWP_DATA_32:
         value = data.dataLo;
         break;
      case latte::pm4::EWP_DATA_64:
         value = static_cast<uint64_t>(data.dataLo) | (static_cast<uint64_t>(data.dataHi) << 32);
         break;
      case latte::pm4::EWP_DATA_CLOCK:
         value = gpu::clock::now();
         break;
      }

      // Swap value
      value = latte::applyEndianSwap(value, data.addrLo.ENDIAN_SWAP());

      // Write value
      switch (data.addrHi.DATA_SEL()) {
      case latte::pm4::EWP_DATA_32:
         *reinterpret_cast<uint32_t *>(ptr) = static_cast<uint32_t>(value);
         break;
      case latte::pm4::EWP_DATA_64:
      case latte::pm4::EWP_DATA_CLOCK:
         *reinterpret_cast<uint64_t *>(ptr) = value;
         break;
      }
   }

   // Generate interrupt if required
   if (data.addrHi.INT_SEL() != latte::pm4::EWP_INT_NONE) {
      auto interrupt = gpu::ih::Entry { };
      interrupt.word0 = latte::CP_INT_SRC_ID::CP_EOP_EVENT;
      gpu::ih::write(interrupt);
   }
}

void
GLDriver::pfpSyncMe(const latte::pm4::PfpSyncMe &data)
{
   // TODO: do we need to do anything?
}

void
GLDriver::setPredication(const latte::pm4::SetPredication &data)
{
   // TODO: Implement me
}

void
GLDriver::executeBuffer(const gpu::ringbuffer::Buffer &buffer)
{
   // Run any remote tasks first
   runRemoteThreadTasks();

   // Execute command buffer
   runCommandBuffer(buffer);
}

void
GLDriver::addFenceSync(std::function<void()> func)
{
   auto sync = gl::glFenceSync(gl::GL_SYNC_GPU_COMMANDS_COMPLETE, static_cast<gl::UnusedMask>(0));
   mSyncList.emplace_back(sync, func);
}

void
GLDriver::addQuerySync(gl::GLuint query, std::function<void()> func)
{
   mSyncList.emplace_back(query, func);
}

void
GLDriver::checkSyncObjects(gl::GLuint64 timeout)
{
   if (mSyncList.empty()) {
      return;
   }

   // Ensure that all sync commands have been sent to the server.
   gl::glFlush();

   bool anyComplete = false;

   // Iterate over the list from newest to oldest, so we never call a
   //  fence function without also calling all previous fence functions
   //  (as could happen if syncs complete while we're iterating forwards).
   for (auto i = mSyncList.rbegin(); i != mSyncList.rend(); ++i) {
      if (&*i == &mSyncList.front() && !anyComplete && timeout > 0) {
         switch (i->type) {
         case SyncObject::FENCE:
            gl::GLenum result;
            result = gl::glClientWaitSync(i->sync, static_cast<gl::SyncObjectMask>(0), timeout);
            i->isComplete = (result != gl::GL_TIMEOUT_EXPIRED);
            break;
         case SyncObject::QUERY: {
            gl::GLint isReady = static_cast<gl::GLint>(gl::GL_TRUE);
            gl::glGetQueryObjectiv(mOccQuery, gl::GL_QUERY_RESULT_AVAILABLE, &isReady);
            if (isReady) {
               i->isComplete = true;
            } else {
               std::this_thread::sleep_for(std::chrono::nanoseconds(timeout));
            }
            break;
         }
         }
      } else {
         switch (i->type) {
         case SyncObject::FENCE: {
            gl::GLint syncState;
            syncState = static_cast<gl::GLint>(gl::GL_SIGNALED);  // avoid hanging on error
            gl::glGetSynciv(i->sync, gl::GL_SYNC_STATUS, sizeof(syncState), nullptr, &syncState);
            i->isComplete = (syncState == gl::GL_SIGNALED);
            break;
         }
         case SyncObject::QUERY: {
            gl::GLint isReady = static_cast<int>(gl::GL_TRUE);
            gl::glGetQueryObjectiv(mOccQuery, gl::GL_QUERY_RESULT_AVAILABLE, &isReady);
            i->isComplete = !!isReady;
            break;
         }
         }
      }
      anyComplete |= i->isComplete;
   }

   // Now finalize and remove all signaled syncs from oldest to newest.
   for (auto i = mSyncList.begin(); i != mSyncList.end(); ) {
      if (i->isComplete) {
         i->func();
         i = mSyncList.erase(i);
      } else {
         ++i;
      }
   }
}

void
GLDriver::runOnGLThread(std::function<void()> func)
{
   std::unique_lock<std::mutex> lock(mTaskListMutex);

   mTaskList.emplace_back(func);
   auto taskIterator = mTaskList.end();
   --taskIterator;

   gpu::ringbuffer::wake();
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
GLDriver::notifyCpuFlush(phys_addr address,
                         uint32_t size)
{
   std::unique_lock<std::mutex> lock(mResourceMap.getMutex());
   auto iter = mResourceMap.getIterator(phys_addr { address }, size);

   Resource *resource;
   while ((resource = iter.next()) != nullptr) {
      resource->dirtyMemory = true;
   }
}

void
GLDriver::notifyGpuFlush(phys_addr address,
                         uint32_t size)
{
   std::unique_lock<std::mutex> lock(mOutputBufferMap.getMutex());

   auto memStart = phys_addr { address };
   auto memEnd = memStart + size;
   auto iter = mOutputBufferMap.getIterator(memStart, size);

   // This allows us to avoid downloading a buffer twice if we hit both its
   //  endpoints in the scan (as will happen if memStart/memEnd coincide
   //  with a buffer's range).
   auto flushStamp = ++mGpuFlushCounter;

   Resource *resource;
   while ((resource = iter.next()) != nullptr) {
      decaf_check(resource->type == Resource::DATA_BUFFER);
      DataBuffer *buffer = reinterpret_cast<DataBuffer *>(resource);

      if (buffer->lastGpuFlush != flushStamp) {
         buffer->lastGpuFlush = flushStamp;

         auto copyOffset = std::max(memStart, buffer->cpuMemStart) - buffer->cpuMemStart;
         auto copySize = (std::min(memEnd, buffer->cpuMemEnd) - buffer->cpuMemStart) - copyOffset;

         runOnGLThread([=](){
            downloadDataBuffer(buffer, copyOffset, copySize);
         });

         buffer->dirtyMemory = false;
      }
   }
}

void
GLDriver::runUntilFlip()
{
   auto startingSwap = mLastSwap;

   if (mRunState == RunState::None) {
      mRunState = RunState::Running;
   }

   checkSyncObjects(0);
   while (true) {
      gpu::ringbuffer::wait();

      auto buffer = gpu::ringbuffer::read();
      if (buffer.empty()) {
         break;
      }

      executeBuffer(buffer);
      checkSyncObjects(0);

      if (mLastSwap > startingSwap) {
         break;
      }
   }
}

void
GLDriver::run()
{
   if (mRunState != RunState::None) {
      return;
   }

   mRunState = RunState::Running;

   while (mRunState == RunState::Running) {
      if (mSyncList.empty()) {
         gpu::ringbuffer::wait();
      }

      auto buffer = gpu::ringbuffer::read();
      if (!buffer.empty()) {
         executeBuffer(buffer);
         checkSyncObjects(0);
      } else {
         checkSyncObjects(10000);  // 10 usec
      }
   }
}

void
GLDriver::stop()
{
   mRunState = RunState::Stopped;
   gpu::ringbuffer::wake();
}

} // namespace opengl

#endif // ifdef DECAF_GL
