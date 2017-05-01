#pragma once
#ifdef DECAF_GL

#include "glsl2/glsl2_translate.h"
#include "gpu_ringbuffer.h"
#include "gpu_opengldriver.h"
#include "latte/latte_constants.h"
#include "latte/latte_contextstate.h"
#include "latte/latte_pm4_commands.h"
#include "opengl_resource.h"
#include "pm4_processor.h"

#include <chrono>
#include <common/log.h>
#include <common/platform.h>
#include <condition_variable>
#include <exception>
#include <glbinding/gl/gl.h>
#include <gsl.h>
#include <libcpu/mem.h>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

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
   gl::GLuint object = 0;
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

   gl::GLuint object = 0;
   std::vector<Attrib> attribs;
   std::array<AttribBufferCache, latte::MaxAttributes> mAttribBufferCache;
   std::string disassembly;
};

struct VertexShader : public Shader
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   gl::GLuint uniformViewport = 0;
   bool isScreenSpace = false;
   std::array<gl::GLuint, latte::MaxAttributes> attribLocations;
   std::array<uint8_t, 256> outputMap;
   std::array<bool, 16> usedUniformBlocks;
   std::array<bool, 4> usedFeedbackBuffers;
   uint32_t lastUniformUpdate = 0;
   std::string code;
   std::string disassembly;
};

struct PixelShader : public Shader
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   gl::GLuint uniformAlphaRef = 0;
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
   gl::GLuint object = 0;
   FetchShader *fetch = nullptr;
   VertexShader *vertex = nullptr;
   PixelShader *pixel = nullptr;  // Null if rasterization is disabled
   uint64_t fetchKey;
   uint64_t vertexKey;
   uint64_t pixelKey;
};

struct HostSurface
{
   gl::GLuint object = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   uint32_t depth = 0;
   uint32_t degamma = false;
   bool isDepthBuffer = false;
   gl::GLenum swizzleR;
   gl::GLenum swizzleG;
   gl::GLenum swizzleB;
   gl::GLenum swizzleA;
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
   gl::GLuint object = 0;
   uint32_t width;
   uint32_t height;
};

struct DataBuffer : public Resource
{
   gl::GLuint object = 0;
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
   gl::GLuint object = 0;
};

struct FeedbackBufferState
{
   bool bound = false;
   gl::GLuint object;
   uint32_t baseOffset;
   uint32_t currentOffset;
};

struct ColorBufferCache
{
   gl::GLuint object = 0;
   uint32_t mask = 0;
};

struct DepthBufferCache
{
   gl::GLuint object = 0;
   bool depthBound = false;
   bool stencilBound = false;
};

struct UniformBlockCache
{
   gl::GLuint vsObject = 0;
   gl::GLuint psObject = 0;
};

struct TextureCache
{
   gl::GLuint surfaceObject = 0;
   uint32_t word4 = 0;
};

struct SamplerCache
{
   glsl2::SamplerUsage usage;

   gl::GLenum wrapS = gl::GL_REPEAT;
   gl::GLenum wrapT = gl::GL_REPEAT;
   gl::GLenum wrapR = gl::GL_REPEAT;

   gl::GLenum minFilter = gl::GL_NEAREST_MIPMAP_LINEAR;
   gl::GLenum magFilter = gl::GL_LINEAR;

   latte::SQ_TEX_BORDER_COLOR borderColorType = latte::SQ_TEX_BORDER_COLOR::TRANS_BLACK;
   std::array<float, 4> borderColorValue;

   gl::GLenum depthCompareMode = gl::GL_NONE;
   gl::GLenum depthCompareFunc = gl::GL_LEQUAL;

   ufixed_4_6_t minLod;
   ufixed_4_6_t maxLod;
   sfixed_1_5_6_t lodBias;
};

struct GLStateCache
{
   std::array<bool, latte::MaxRenderTargets> blendEnable;

   bool cullFaceEnable = false;
   gl::GLenum cullFace = gl::GL_BACK;
   gl::GLenum frontFace = gl::GL_CCW;

   bool depthEnable = false;
   bool depthWrite = true;
   gl::GLenum depthFunc = gl::GL_LESS;

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
   gl::GLuint primRestartIndex = static_cast<gl::GLuint>(-1);
};

struct RemoteThreadTask
{
   std::function<void()> func;
   std::condition_variable completionCV;

   RemoteThreadTask(std::function<void()> func_) : func(func_)
   {
   }
};

struct SyncObject
{
   enum Type {
      FENCE,
      QUERY,
   } type;

   union {
      gl::GLsync sync;
      gl::GLuint query;
   };
   bool isComplete;
   std::function<void()> func;

   SyncObject(gl::GLsync sync_, std::function<void()> func_) : type(FENCE), sync(sync_), func(func_)
   {
   }

   SyncObject(gl::GLuint query_, std::function<void()> func_) : type(QUERY), query(query_), func(func_)
   {
   }
};

using GLContext = uint64_t;

class GLDriver : public gpu::OpenGLDriver, public Pm4Processor
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

   virtual float
   getAverageFrametime() override;

   virtual GraphicsDebugInfo
   getGraphicsDebugInfo() override;

   virtual void
   notifyCpuFlush(void *ptr,
                  uint32_t size) override;

   virtual void
   notifyGpuFlush(void *ptr,
                  uint32_t size) override;

   virtual void
   getSwapBuffers(gl::GLuint *tv,
                  gl::GLuint *drc) override;

   virtual void
   syncPoll(const SwapFunction &swapFunc) override;

   virtual bool
   startFrameCapture(const std::string &outPrefix,
                     bool captureTV,
                     bool captureDRC) override;

   virtual size_t
   stopFrameCapture() override;

private:
   void initGL();
   void executeBuffer(const gpu::ringbuffer::Item &item);
   uint64_t getGpuClock();

   void decafSetBuffer(const latte::pm4::DecafSetBuffer &data) override;
   void decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data) override;
   void decafSwapBuffers(const latte::pm4::DecafSwapBuffers &data) override;
   void decafCapSyncRegisters(const latte::pm4::DecafCapSyncRegisters &data) override;
   void decafClearColor(const latte::pm4::DecafClearColor &data) override;
   void decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data) override;
   void decafDebugMarker(const latte::pm4::DecafDebugMarker &data) override;
   void decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data) override;
   void decafCopySurface(const latte::pm4::DecafCopySurface &data) override;
   void decafSetSwapInterval(const latte::pm4::DecafSetSwapInterval &data) override;
   void drawIndexAuto(const latte::pm4::DrawIndexAuto &data) override;
   void drawIndex2(const latte::pm4::DrawIndex2 &data) override;
   void drawIndexImmd(const latte::pm4::DrawIndexImmd &data) override;
   void memWrite(const latte::pm4::MemWrite &data) override;
   void eventWrite(const latte::pm4::EventWrite &data) override;
   void eventWriteEOP(const latte::pm4::EventWriteEOP &data) override;
   void pfpSyncMe(const latte::pm4::PfpSyncMe &data) override;
   void streamOutBaseUpdate(const latte::pm4::StreamOutBaseUpdate &data) override;
   void streamOutBufferUpdate(const latte::pm4::StreamOutBufferUpdate &data) override;
   void surfaceSync(const latte::pm4::SurfaceSync &data) override;

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
                     gl::GLenum swizzleR,
                     gl::GLenum swizzleG,
                     gl::GLenum swizzleB,
                     gl::GLenum swizzleA);

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
   beginTransformFeedback(gl::GLenum primitive);

   void
   endTransformFeedback();

   void
   updateTransformFeedbackOffsets();

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
   addFenceSync(std::function<void()> func);

   void
   addQuerySync(gl::GLuint query, std::function<void()> func);

   void
   checkSyncObjects(gl::GLuint64 timeout);

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

   bool
   dumpScanBuffer(const std::string &filename,
                  const ScanBufferChain &buf);

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

   gl::GLuint mBlitFrameBuffers[2];
   gl::GLuint mFrameBuffer;
   gl::GLuint mColorClearFrameBuffer;
   gl::GLuint mDepthClearFrameBuffer;
   ShaderPipeline *mActiveShader = nullptr;
   std::array<gl::GLenum, latte::MaxRenderTargets> mDrawBuffers;
   ScanBufferChain mTvScanBuffers;
   ScanBufferChain mDrcScanBuffers;

   gl::GLuint mFeedbackQuery = 0;
   unsigned int mFeedbackQueryBuffers = 0;
   std::array<unsigned int, latte::MaxStreamOutBuffers> mFeedbackQueryStride;
   bool mFeedbackActive = false;
   gl::GLenum mFeedbackPrimitive;
   std::array<FeedbackBufferState, latte::MaxStreamOutBuffers> mFeedbackBufferState;

   gl::GLuint mOccQuery = 0;
   uint32_t mLastOccQueryAddress = 0;

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

   std::list<SyncObject> mSyncList;

   size_t mFramesCaptured = 0;
   std::string mFrameCapturePrefix;
   bool mFrameCaptureTV = false;
   bool mFrameCaptureDRC = false;
};

} // namespace opengl

#endif // ifdef DECAF_GL
