#pragma once
#include "common/log.h"
#include "common/platform.h"
#include "glsl2_translate.h"
#include "gpu/latte_constants.h"
#include "gpu/latte_contextstate.h"
#include "gpu/pm4_buffer.h"
#include "gpu/pm4_packets.h"
#include "libdecaf/decaf_graphics.h"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <glbinding/gl/gl.h>
#include <gsl.h>
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

struct Resource
{
   //! The start of the CPU memory region this occupies
   uint32_t cpuMemStart;

   //! The end of the CPU memory region this occupies
   uint32_t cpuMemEnd;

   //! Hash of the memory contents, for detecting changes
   uint64_t cpuMemHash[2] = { 0, 0 };

   //! True if a DCFlush has been received for the memory region
   bool dirtyMemory = true;
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

enum class SyncWaitType : uint32_t
{
   Fence,
   Query
};

struct SyncWait
{
   SyncWaitType type;
   union {
      gl::GLsync fence;
      gl::GLuint query;
   };
   std::function<void()> func;
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

   latte::SQ_TEX_BORDER_COLOR borderColorType = latte::SQ_TEX_BORDER_COLOR_TRANS_BLACK;
   std::array<float, 4> borderColorValue;

   gl::GLenum depthCompareMode = gl::GL_NONE;
   gl::GLenum depthCompareFunc = gl::GL_LEQUAL;

   ufixed_4_6_t minLod;
   ufixed_4_6_t maxLod;
   sfixed_6_6_t lodBias;
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
   gl::GLuint primRestartIndex;
};

struct RemoteThreadTask
{
   std::function<void()> func;
   std::condition_variable completionCV;

   RemoteThreadTask(std::function<void()> func) : func(func)
   {
   }
};

using GLContext = uint64_t;

class GLDriver : public decaf::OpenGLDriver
{
public:
   virtual ~GLDriver() = default;

   virtual void run() override;
   virtual void stop() override;

   virtual float getAverageFPS() override;

   virtual void notifyCpuFlush(void *ptr, uint32_t size) override;
   virtual void notifyGpuFlush(void *ptr, uint32_t size) override;

   virtual void getSwapBuffers(unsigned int *tv, unsigned int *drc) override;
   virtual void syncPoll(const SwapFunction &swapFunc) override;

private:
   void initGL();
   void executeBuffer(pm4::Buffer *buffer);

   uint64_t getGpuClock();

   void handlePacketType0(pm4::type0::Header header, const gsl::span<uint32_t> &data);
   void handlePacketType3(pm4::type3::Header header, const gsl::span<uint32_t> &data);
   void decafSetBuffer(const pm4::DecafSetBuffer &data);
   void decafCopyColorToScan(const pm4::DecafCopyColorToScan &data);
   void decafSwapBuffers(const pm4::DecafSwapBuffers &data);
   void decafClearColor(const pm4::DecafClearColor &data);
   void decafClearDepthStencil(const pm4::DecafClearDepthStencil &data);
   void decafDebugMarker(const pm4::DecafDebugMarker &data);
   void decafOSScreenFlip(const pm4::DecafOSScreenFlip &data);
   void decafCopySurface(const pm4::DecafCopySurface &data);
   void decafSetSwapInterval(const pm4::DecafSetSwapInterval &data);
   void drawIndexAuto(const pm4::DrawIndexAuto &data);
   void drawIndex2(const pm4::DrawIndex2 &data);
   void drawIndexImmd(const pm4::DrawIndexImmd &data);
   void indexType(const pm4::IndexType &data);
   void indirectBufferCall(const pm4::IndirectBufferCall &data);
   void numInstances(const pm4::NumInstances &data);
   void memWrite(const pm4::MemWrite &data);
   void nopPacket(const pm4::Nop &data);
   void eventWrite(const pm4::EventWrite &data);
   void eventWriteEOP(const pm4::EventWriteEOP &data);
   void pfpSyncMe(const pm4::PfpSyncMe &data);
   void streamOutBaseUpdate(const pm4::StreamOutBaseUpdate &data);
   void streamOutBufferUpdate(const pm4::StreamOutBufferUpdate &data);
   void surfaceSync(const pm4::SurfaceSync &data);
   void contextControl(const pm4::ContextControl &data);

   void setAluConsts(const pm4::SetAluConsts &data);
   void setConfigRegs(const pm4::SetConfigRegs &data);
   void setContextRegs(const pm4::SetContextRegs &data);
   void setControlConstants(const pm4::SetControlConstants &data);
   void setLoopConsts(const pm4::SetLoopConsts &data);
   void setSamplers(const pm4::SetSamplers &data);
   void setResources(const pm4::SetResources &data);

   void loadAluConsts(const pm4::LoadAluConst &data);
   void loadBoolConsts(const pm4::LoadBoolConst &data);
   void loadConfigRegs(const pm4::LoadConfigReg &data);
   void loadContextRegs(const pm4::LoadContextReg &data);
   void loadControlConstants(const pm4::LoadControlConst &data);
   void loadLoopConsts(const pm4::LoadLoopConst &data);
   void loadSamplers(const pm4::LoadSampler &data);
   void loadResources(const pm4::LoadResource &data);
   void loadRegisters(latte::Register base,
                      be_val<uint32_t> *src,
                      const gsl::span<std::pair<uint32_t, uint32_t>> &registers);

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

   void beginTransformFeedback(gl::GLenum primitive);
   void endTransformFeedback();

   void setRegister(latte::Register reg, uint32_t value);
   void applyRegister(latte::Register reg);

   int countModifiedUniforms(latte::Register firstReg,
                             uint32_t lastUniformUpdate);

   bool parseFetchShader(FetchShader &shader, void *buffer, size_t size);
   bool compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size, bool isScreenSpace);
   bool compilePixelShader(PixelShader &pixel, VertexShader &vertex, uint8_t *buffer, size_t size);

   void injectFence(std::function<void()> func);
   void checkSyncObjects();

   void runCommandBuffer(uint32_t *buffer, uint32_t size);

   void runOnGLThread(std::function<void()> func);
   void runRemoteThreadTasks();

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }

   void drawPrimitives(uint32_t count,
                       const void *indices,
                       latte::VGT_INDEX indexFmt);

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

   std::array<uint32_t, 0x10000> mRegisters;

   bool mViewportDirty = false;
   bool mDepthRangeDirty = false;
   bool mScissorDirty = false;

   // Protects resource lists; used by notifyCpuFlush() to safely set dirty flags
   std::mutex mResourceMutex;

   std::unordered_map<uint64_t, FetchShader *> mFetchShaders;  // Protected by mResourceMutex
   std::unordered_map<uint64_t, VertexShader *> mVertexShaders;  // Protected by mResourceMutex
   std::unordered_map<uint64_t, PixelShader *> mPixelShaders;  // Protected by mResourceMutex
   std::map<ShaderPipelineKey, ShaderPipeline> mShaderPipelines;  // Not touched by notifyCpuFlush()
   std::unordered_map<uint64_t, SurfaceBuffer> mSurfaces;  // Protected by mResourceMutex
   std::unordered_map<uint32_t, DataBuffer> mDataBuffers;  // Protected by mResourceMutex

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

   std::queue<SyncWait> mSyncWaits;

   gl::GLuint mFeedbackQuery = 0;
   bool mFeedbackActive = false;
   gl::GLenum mFeedbackPrimitive;
   std::array<FeedbackBufferState, latte::MaxStreamOutBuffers> mFeedbackBufferState;

   gl::GLuint mOccQuery = 0;
   uint64_t mTotalSamplesPassed = 0;

   latte::ShadowState mShadowState;

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

#ifdef PLATFORM_WINDOWS
   uint64_t mDeviceContext = 0;
   uint64_t mOpenGLContext = 0;
#endif
};

} // namespace opengl

} // namespace gpu
