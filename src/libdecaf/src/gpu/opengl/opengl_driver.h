#pragma once

#ifndef DECAF_NOGL

#include "gpu/glsl2/glsl2_translate.h"
#include "gpu/latte_constants.h"
#include "gpu/latte_contextstate.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_packets.h"
#include "gpu/pm4_processor.h"
#include "libdecaf/decaf_graphics.h"
#include "libdecaf/decaf_opengl.h"
#include "opengl_resource.h"

#include <chrono>
#include <common/gl.h>
#include <common/log.h>
#include <common/platform.h>
#include <condition_variable>
#include <exception>
#include <gsl.h>
#include <libcpu/mem.h>
#include <list>
#include <map>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace gpu
{

namespace opengl
{

enum class SurfaceUseState : uint32_t
{
   None,
   CpuWritten,
   GpuWritten
};

struct AttribBufferCache
{
   GLuint object = 0;
   uint32_t stride = 0;
};

struct Shader : public Resource
{
   //! True if the shader needs to be rebuilt due to dirtyMemory
   bool needRebuild = false;

   //! Number of references from ShaderPipelines (used for garbage collection)
   unsigned refCount = 0;

   Shader() : Resource(Resource::SHADER) { }
};

struct FetchShader : public Shader
{
   struct Attrib
   {
      uint32_t buffer;
      uint32_t offset;
      uint32_t location;
      uint32_t bytesPerElement;
      latte::SQ_SEL srcSelX;
      latte::SQ_VTX_FETCH_TYPE type;
      latte::SQ_DATA_FORMAT format;
      latte::SQ_SEL dstSel[4];
      latte::SQ_NUM_FORMAT numFormat;
      latte::SQ_ENDIAN endianSwap;
      latte::SQ_FORMAT_COMP formatComp;
   };

   GLuint object = 0;
   std::vector<Attrib> attribs;
   std::array<AttribBufferCache, latte::MaxAttributes> mAttribBufferCache;
   std::string disassembly;
};

struct VertexShader : public Shader
{
   GLuint object = 0;
   GLuint uniformRegisters = 0;
   GLuint uniformViewport = 0;
   bool isScreenSpace = false;
   std::array<GLuint, latte::MaxAttributes> attribLocations;
   std::array<uint8_t, 256> outputMap;
   std::array<bool, 16> usedUniformBlocks;
   std::array<bool, 4> usedFeedbackBuffers;
   uint32_t lastUniformUpdate = 0;
   std::string code;
   std::string disassembly;
};

struct PixelShader : public Shader
{
   GLuint object = 0;
   GLuint uniformRegisters = 0;
   GLuint uniformAlphaRef = 0;
   latte::SX_ALPHA_TEST_CONTROL sx_alpha_test_control;
   std::array<glsl2::SamplerUsage, latte::MaxSamplers> samplerUsage;
   std::array<bool, 16> usedUniformBlocks;
   uint32_t lastUniformUpdate = 0;
   std::string code;
   std::string disassembly;
};

using ShaderPipelineKey = std::tuple<uint64_t, uint64_t, uint64_t>;

struct ShaderPipeline
{
   GLuint object = 0;
   FetchShader *fetch = nullptr;
   VertexShader *vertex = nullptr;
   PixelShader *pixel = nullptr;  // Null if rasterization is disabled
   uint64_t fetchKey;
   uint64_t vertexKey;
   uint64_t pixelKey;
};

struct HostSurface
{
   GLuint object = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   uint32_t depth = 0;
   uint32_t degamma = false;
   bool isDepthBuffer = false;
   GLenum swizzleR;
   GLenum swizzleG;
   GLenum swizzleB;
   GLenum swizzleA;
   HostSurface *next = nullptr;
};

struct SurfaceBuffer : public Resource
{
   HostSurface *active = nullptr;
   HostSurface *master = nullptr;
   SurfaceUseState state = SurfaceUseState::None;
   bool needUpload = true;
   struct {
      latte::SQ_TEX_DIM dim;
      latte::SQ_DATA_FORMAT format;
      latte::SQ_NUM_FORMAT numFormat;
      latte::SQ_FORMAT_COMP formatComp;
      uint32_t degamma;
   } dbgInfo;

   SurfaceBuffer() : Resource(Resource::SURFACE) { }
};

struct ScanBufferChain
{
   GLuint object = 0;
   uint32_t width;
   uint32_t height;
};

struct DataBuffer : public Resource
{
   GLuint object = 0;
   uint32_t allocatedSize = 0;
   void *mappedBuffer = nullptr;
   bool isInput = false;  // Uniform or attribute buffers
   bool isOutput = false;  // Transform feedback buffers
   bool dirtyMap = false;  // True if we need to glFlushMappedBufferRange
   uint32_t lastGpuFlush = 0;  // Last time we touched this buffer in notifyGpuFlush()

   DataBuffer() : Resource(Resource::DATA_BUFFER) { }
};

struct Sampler
{
   GLuint object = 0;
};

struct FeedbackBufferState
{
   bool bound = false;
   GLuint object;
   uint32_t baseOffset;
   uint32_t currentOffset;
};

enum class SyncWaitType : uint32_t
{
   Fence,
   Query
};

struct SyncWait
{
   SyncWaitType type;
   union {
      GLsync fence;
      GLuint query;
   };
   std::function<void()> func;
};

struct ColorBufferCache
{
   GLuint object = 0;
   uint32_t mask = 0;
};

struct DepthBufferCache
{
   GLuint object = 0;
   bool depthBound = false;
   bool stencilBound = false;
};

struct UniformBlockCache
{
   GLuint vsObject = 0;
   GLuint psObject = 0;
};

struct TextureCache
{
   GLuint surfaceObject = 0;
   uint32_t word4 = 0;
};

struct SamplerCache
{
   glsl2::SamplerUsage usage;

   GLenum wrapS = GL_REPEAT;
   GLenum wrapT = GL_REPEAT;
   GLenum wrapR = GL_REPEAT;

   GLenum minFilter = GL_NEAREST_MIPMAP_LINEAR;
   GLenum magFilter = GL_LINEAR;

   latte::SQ_TEX_BORDER_COLOR borderColorType = latte::SQ_TEX_BORDER_COLOR::TRANS_BLACK;
   std::array<float, 4> borderColorValue;

   GLenum depthCompareMode = GL_NONE;
   GLenum depthCompareFunc = GL_LEQUAL;

   ufixed_4_6_t minLod;
   ufixed_4_6_t maxLod;
   sfixed_6_6_t lodBias;
};

struct GLStateCache
{
   std::array<bool, latte::MaxRenderTargets> blendEnable;

   bool cullFaceEnable = false;
   GLenum cullFace = GL_BACK;
   GLenum frontFace = GL_CCW;

   bool depthEnable = false;
   bool depthWrite = true;
   GLenum depthFunc = GL_LESS;

   bool stencilEnable = false;
   uint32_t stencilState = 0;
   uint8_t frontStencilRef = 0;
   uint8_t frontStencilMask = 255;
   uint8_t backStencilRef = 0;
   uint8_t backStencilMask = 255;

   bool rasterizerDiscard = false;
   bool depthClamp = false;  // See note in initGL()
   bool halfZClipSpace = false;

   bool primRestartEnable = false;  // See note in initGL()
   GLuint primRestartIndex = static_cast<GLuint>(-1);
};

struct RemoteThreadTask
{
   std::function<void()> func;
   std::condition_variable completionCV;

   RemoteThreadTask(std::function<void()> func_) : func(func_)
   {
   }
};

using GLContext = uint64_t;

class GLDriver : public decaf::OpenGLDriver, public Pm4Processor
{
public:
   GLDriver();
   virtual ~GLDriver() = default;

   virtual void
   run() override;

   virtual void
   stop() override;

   virtual float
   getAverageFPS() override;

   virtual void
   notifyCpuFlush(void *ptr,
                  uint32_t size) override;

   virtual void
   notifyGpuFlush(void *ptr,
                  uint32_t size) override;

   virtual void
   getSwapBuffers(unsigned int *tv,
                  unsigned int *drc) override;

   virtual void
   syncPoll(const SwapFunction &swapFunc) override;

private:
   void initGL();
   void executeBuffer(pm4::Buffer *buffer);
   uint64_t getGpuClock();

   void decafSetBuffer(const pm4::DecafSetBuffer &data) override;
   void decafCopyColorToScan(const pm4::DecafCopyColorToScan &data) override;
   void decafSwapBuffers(const pm4::DecafSwapBuffers &data) override;
   void decafCapSyncRegisters(const pm4::DecafCapSyncRegisters &data) override;
   void decafClearColor(const pm4::DecafClearColor &data) override;
   void decafClearDepthStencil(const pm4::DecafClearDepthStencil &data) override;
   void decafDebugMarker(const pm4::DecafDebugMarker &data) override;
   void decafOSScreenFlip(const pm4::DecafOSScreenFlip &data) override;
   void decafCopySurface(const pm4::DecafCopySurface &data) override;
   void decafSetSwapInterval(const pm4::DecafSetSwapInterval &data) override;
   void drawIndexAuto(const pm4::DrawIndexAuto &data) override;
   void drawIndex2(const pm4::DrawIndex2 &data) override;
   void drawIndexImmd(const pm4::DrawIndexImmd &data) override;
   void memWrite(const pm4::MemWrite &data) override;
   void eventWrite(const pm4::EventWrite &data) override;
   void eventWriteEOP(const pm4::EventWriteEOP &data) override;
   void pfpSyncMe(const pm4::PfpSyncMe &data) override;
   void streamOutBaseUpdate(const pm4::StreamOutBaseUpdate &data) override;
   void streamOutBufferUpdate(const pm4::StreamOutBufferUpdate &data) override;
   void surfaceSync(const pm4::SurfaceSync &data) override;

   void applyRegister(latte::Register reg) override;

   void
   uploadSurface(SurfaceBuffer *surface,
                 ppcaddr_t baseAddress,
                 uint32_t swizzle,
                 uint32_t pitch,
                 uint32_t width,
                 uint32_t height,
                 uint32_t depth,
                 uint32_t samples,
                 latte::SQ_TEX_DIM dim,
                 latte::SQ_DATA_FORMAT format,
                 latte::SQ_NUM_FORMAT numFormat,
                 latte::SQ_FORMAT_COMP formatComp,
                 uint32_t degamma,
                 bool isDepthBuffer,
                 latte::SQ_TILE_MODE tileMode);

   SurfaceBuffer *
   getSurfaceBuffer(ppcaddr_t baseAddress,
                    uint32_t pitch,
                    uint32_t width,
                    uint32_t height,
                    uint32_t depth,
                    uint32_t samples,
                    latte::SQ_TEX_DIM dim,
                    latte::SQ_DATA_FORMAT format,
                    latte::SQ_NUM_FORMAT numFormat,
                    latte::SQ_FORMAT_COMP formatComp,
                    uint32_t degamma,
                    bool isDepthBuffer,
                    latte::SQ_TILE_MODE tileMode,
                    bool forWrite,
                    bool discardData);

   void
   setSurfaceSwizzle(SurfaceBuffer *surface,
                     GLenum swizzleR,
                     GLenum swizzleG,
                     GLenum swizzleB,
                     GLenum swizzleA);

   SurfaceBuffer *
   getColorBuffer(latte::CB_COLORN_BASE base,
                  latte::CB_COLORN_SIZE size,
                  latte::CB_COLORN_INFO info,
                  bool discardData);

   SurfaceBuffer *
   getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                  latte::DB_DEPTH_SIZE db_depth_size,
                  latte::DB_DEPTH_INFO db_depth_info,
                  bool discardData);

   bool checkReadyDraw();
   bool checkActiveAttribBuffers();
   bool checkActiveColorBuffer();
   bool checkActiveDepthBuffer();
   bool checkActiveFeedbackBuffers();
   bool checkActiveSamplers();
   bool checkActiveShader();
   bool checkActiveTextures();
   bool checkActiveUniforms();
   bool checkAttribBuffersBound();
   bool checkViewport();

   DataBuffer *
   getDataBuffer(uint32_t address,
                 uint32_t size,
                 bool isInput,
                 bool isOutput);
   void
   uploadDataBuffer(DataBuffer *buffer,
                    uint32_t offset,
                    uint32_t size);
   void
   downloadDataBuffer(DataBuffer *buffer,
                      uint32_t offset,
                      uint32_t size);

   void
   beginTransformFeedback(GLenum primitive);

   void
   endTransformFeedback();

   int
   countModifiedUniforms(latte::Register firstReg,
                         uint32_t lastUniformUpdate);

   bool
   parseFetchShader(FetchShader &shader,
                    void *buffer,
                    size_t size);

   bool
   compileVertexShader(VertexShader &vertex,
                       FetchShader &fetch,
                       uint8_t *buffer,
                       size_t size,
                       bool isScreenSpace);

   bool
   compilePixelShader(PixelShader &pixel,
                      VertexShader &vertex,
                      uint8_t *buffer,
                      size_t size);

   void
   injectFence(std::function<void()> func);

   void
   checkSyncObjects();

   void
   runOnGLThread(std::function<void()> func);

   void
   runRemoteThreadTasks();

   void
   drawPrimitives(uint32_t count,
                  const void *indices,
                  latte::VGT_INDEX_TYPE indexFmt);

   void
   drawPrimitivesIndexed(const void *indices,
                         uint32_t count);

private:
   enum class RunState
   {
      None,
      Running,
      Stopped
   };

   volatile RunState mRunState = RunState::None;
   std::thread mThread;
   unsigned mSwapInterval = 1;
   SwapFunction mSwapFunc;

   bool mViewportDirty = false;
   bool mDepthRangeDirty = false;
   bool mScissorDirty = false;

   std::unordered_map<uint64_t, FetchShader *> mFetchShaders;
   std::unordered_map<uint64_t, VertexShader *> mVertexShaders;
   std::unordered_map<uint64_t, PixelShader *> mPixelShaders;
   std::map<ShaderPipelineKey, ShaderPipeline> mShaderPipelines;
   std::unordered_map<uint64_t, SurfaceBuffer> mSurfaces;
   std::unordered_map<uint32_t, DataBuffer> mDataBuffers;

   ResourceMemoryMap mResourceMap;
   ResourceMemoryMap mOutputBufferMap;
   uint32_t mGpuFlushCounter = 0;

   std::array<Sampler, latte::MaxSamplers> mVertexSamplers;
   std::array<Sampler, latte::MaxSamplers> mPixelSamplers;
   std::array<Sampler, latte::MaxSamplers> mGeometrySamplers;

   GLuint mBlitFrameBuffers[2];
   GLuint mFrameBuffer;
   GLuint mColorClearFrameBuffer;
   GLuint mDepthClearFrameBuffer;
   ShaderPipeline *mActiveShader = nullptr;
   std::array<GLenum, latte::MaxRenderTargets> mDrawBuffers;
   ScanBufferChain mTvScanBuffers;
   ScanBufferChain mDrcScanBuffers;

   std::queue<SyncWait> mSyncWaits;

   GLuint mFeedbackQuery = 0;
   bool mFeedbackActive = false;
   GLenum mFeedbackPrimitive;
   std::array<FeedbackBufferState, latte::MaxStreamOutBuffers> mFeedbackBufferState;

   GLuint mOccQuery = 0;
   uint64_t mTotalSamplesPassed = 0;

   GLStateCache mGLStateCache;
   bool mFramebufferChanged = false;
   std::array<ColorBufferCache, latte::MaxRenderTargets> mColorBufferCache;
   DepthBufferCache mDepthBufferCache;
   std::array<UniformBlockCache, latte::MaxUniformBlocks> mUniformBlockCache;
   std::array<TextureCache, latte::MaxTextures> mPixelTextureCache;
   std::array<SamplerCache, latte::MaxSamplers> mPixelSamplerCache;

   // Used to detect changes to uniform registers; see countModifiedUniforms()
   uint32_t mUniformUpdateGen = 0;
   std::array<uint32_t, (2 * latte::MaxUniformRegisters) / 16> mLastUniformUpdate;

   using duration_system_clock = std::chrono::duration<double, std::chrono::system_clock::period>;
   using duration_ms = std::chrono::duration<double, std::chrono::milliseconds::period>;
   std::chrono::time_point<std::chrono::system_clock> mLastSwap;
   duration_system_clock mAverageFrameTime;

   std::mutex mTaskListMutex;  // Protects mTaskList
   std::list<RemoteThreadTask> mTaskList;

};

} // namespace opengl

} // namespace gpu

#endif // DECAF_NOGL
