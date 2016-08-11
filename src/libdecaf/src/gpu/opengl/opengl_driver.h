#pragma once
#include "common/platform.h"
#include "common/log.h"
#include "gpu/pm4.h"
#include "gpu/latte_constants.h"
#include "gpu/latte_contextstate.h"
#include "gpu/pm4_buffer.h"
#include "libdecaf/decaf_graphics.h"
#include <chrono>
#include <condition_variable>
#include <exception>
#include <glbinding/gl/types.h>
#include <gsl.h>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace gpu
{

namespace opengl
{

enum class SamplerType
{
   Invalid,
   Sampler1D,
   Sampler2D,
   Sampler2DArray,
};

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
};

struct FetchShader : public Resource
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
   std::string disassembly;
};

struct VertexShader : public Resource
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   gl::GLuint uniformTexScale = 0;
   std::array<gl::GLuint, latte::MaxAttributes> attribLocations;
   std::array<uint8_t, 256> outputMap;
   std::array<bool, 16> usedUniformBlocks;
   std::array<bool, 4> usedFeedbackBuffers;
   std::string code;
   std::string disassembly;
};

struct PixelShader : public Resource
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   gl::GLuint uniformAlphaRef = 0;
   gl::GLuint uniformTexScale = 0;
   std::array<SamplerType, latte::MaxSamplers> samplerTypes;
   latte::SX_ALPHA_TEST_CONTROL sx_alpha_test_control;
   std::array<bool, 16> usedUniformBlocks;
   std::string code;
   std::string disassembly;
};

using ShaderKey = std::tuple<uint64_t, uint64_t, uint64_t>;

struct Shader
{
   gl::GLuint object = 0;
   FetchShader *fetch;
   VertexShader *vertex;
   PixelShader *pixel;  // Null indicates a transform-feedback shader
   uint64_t fetchKey;
   uint64_t vertexKey;
   uint64_t pixelKey;
};

struct SurfaceBuffer : Resource
{
   gl::GLuint object = 0;
   uint32_t width = 0;
   uint32_t height = 0;
   uint32_t depth = 0;
   SurfaceUseState state = SurfaceUseState::None;
   bool dirtyAsTexture = true;
   uint64_t cpuMemHash[2] = { 0 };
   struct {
      uint32_t depth = 0;
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

struct AttributeBuffer : public Resource
{
   gl::GLuint object = 0;
   uint32_t allocatedSize = 0;
   void *mappedBuffer = nullptr;
   bool dirtyAsBuffer = true;
   uint64_t cpuMemHash[2] = { 0 };
};

struct FeedbackBuffer
{
   gl::GLuint object = 0;
   uint32_t size = 0;
   uint32_t addr = 0;
};

struct Sampler
{
   gl::GLuint object = 0;
};

struct UniformBuffer : public Resource
{
   gl::GLuint object = 0;
   uint32_t allocatedSize = 0;
   bool dirtyAsBuffer = true;
   uint64_t cpuMemHash[2] = { 0 };
};

struct ColorBufferCache
{
   uint32_t base = 0;
   uint32_t size = 0;
   uint32_t info = 0;
   uint32_t mask = 0;
};

struct DepthBufferCache
{
   uint32_t base = 0;
   uint32_t size = 0;
   uint32_t info = 0;
};

struct TextureCache
{
   uint32_t baseAddress = 0;
   uint32_t word0 = 0;
   uint32_t word1 = 0;
   uint32_t word2 = 0;
   uint32_t word3 = 0;
   uint32_t word4 = 0;
   uint32_t word5 = 0;
   uint32_t word6 = 0;
};

struct SamplerCache
{
   uint32_t word0 = 0;
   uint32_t word1 = 0;
   uint32_t word2 = 0;
   bool depthCompare = false;  // TODO: might be unnecessary; see TODO note in checkActiveSamplers()
};

struct FeedbackBufferCache
{
   bool enable = false;
};

using GLContext = uint64_t;

class GLDriver : public decaf::OpenGLDriver
{
public:
   virtual ~GLDriver() = default;

   virtual void run() override;
   virtual void stop() override;
   virtual float getAverageFPS() override;
   virtual void getSwapBuffers(unsigned int *tv, unsigned int *drc) override;
   virtual void syncPoll(std::function<void(unsigned int, unsigned int)> swapFunc) override;

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
   void decafSetContextState(const pm4::DecafSetContextState &data);
   void decafInvalidate(const pm4::DecafInvalidate &data);
   void decafDebugMarker(const pm4::DecafDebugMarker &data);
   void decafOSScreenFlip(const pm4::DecafOSScreenFlip &data);
   void drawIndexAuto(const pm4::DrawIndexAuto &data);
   void drawIndex2(const pm4::DrawIndex2 &data);
   void indexType(const pm4::IndexType &data);
   void indirectBufferCall(const pm4::IndirectBufferCall &data);
   void numInstances(const pm4::NumInstances &data);
   void memWrite(const pm4::MemWrite &data);
   void eventWrite(const pm4::EventWrite &data);
   void eventWriteEOP(const pm4::EventWriteEOP &data);
   void handlePendingEOP();
   void pfpSyncMe(const pm4::PfpSyncMe &data);
   void streamOutBaseUpdate(const pm4::StreamOutBaseUpdate &data);
   void streamOutBufferUpdate(const pm4::StreamOutBufferUpdate &data);
   void nopPacket(const pm4::Nop &data);

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
                      uint32_t *src,
                      const gsl::span<std::pair<uint32_t, uint32_t>> &registers);

   SurfaceBuffer *
   getSurfaceBuffer(ppcaddr_t baseAddress,
                    uint32_t pitch,
                    uint32_t width,
                    uint32_t height,
                    uint32_t depth,
                    latte::SQ_TEX_DIM dim,
                    latte::SQ_DATA_FORMAT format,
                    latte::SQ_NUM_FORMAT numFormat,
                    latte::SQ_FORMAT_COMP formatComp,
                    uint32_t degamma, bool isDepthBuffer);

   SurfaceBuffer *
   getColorBuffer(latte::CB_COLORN_BASE base,
                  latte::CB_COLORN_SIZE size,
                  latte::CB_COLORN_INFO info);

   SurfaceBuffer *
   getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                  latte::DB_DEPTH_SIZE db_depth_size,
                  latte::DB_DEPTH_INFO db_depth_info);

   bool checkReadyDraw();
   bool checkActiveAttribBuffers();
   bool checkActiveColorBuffer();
   bool checkActiveDepthBuffer();
   bool checkActiveFeedbackBuffers();
   bool checkActiveSamplers();
   bool checkActiveShader();
   bool checkActiveTextures();
   bool checkActiveUniforms();
   bool checkViewport();

   void createFeedbackBuffer(unsigned index);
   void copyFeedbackBuffer(unsigned index);

   void setRegister(latte::Register reg, uint32_t value);
   void applyRegister(latte::Register reg);

   bool parseFetchShader(FetchShader &shader, void *buffer, size_t size);
   bool compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size);
   bool compilePixelShader(PixelShader &pixel, VertexShader &vertex, uint8_t *buffer, size_t size);

   void runCommandBuffer(uint32_t *buffer, uint32_t size);

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }

   void drawPrimitives(uint32_t count,
                       const void *indices,
                       latte::VGT_INDEX indexFmt);

private:
   enum class RunState {
      None,
      Running,
      Stopped
   };

   volatile RunState mRunState = RunState::None;
   std::thread mThread;
   std::function<void(unsigned int, unsigned int)> mSwapFunc;

   std::array<uint32_t, 0x10000> mRegisters;

   bool mViewportDirty = false;
   bool mScissorDirty = false;

   std::unordered_map<uint64_t, FetchShader> mFetchShaders;
   std::unordered_map<uint64_t, VertexShader> mVertexShaders;
   std::unordered_map<uint64_t, PixelShader> mPixelShaders;
   std::map<ShaderKey, Shader> mShaders;
   std::unordered_map<uint64_t, SurfaceBuffer> mSurfaces;
   std::unordered_map<uint32_t, AttributeBuffer> mAttribBuffers;
   std::unordered_map<uint32_t, UniformBuffer> mUniformBuffers;
   std::unordered_map<uint32_t, FeedbackBuffer> mFeedbackBuffers;

   std::array<Sampler, latte::MaxSamplers> mVertexSamplers;
   std::array<Sampler, latte::MaxSamplers> mPixelSamplers;
   std::array<Sampler, latte::MaxSamplers> mGeometrySamplers;

   std::array<float, latte::MaxTextures * 4> mTexCoordScale;

   gl::GLuint mBlitFrameBuffers[2];
   gl::GLuint mFrameBuffer;
   gl::GLuint mColorClearFrameBuffer;
   gl::GLuint mDepthClearFrameBuffer;
   Shader *mActiveShader = nullptr;
   SurfaceBuffer *mActiveDepthBuffer = nullptr;
   std::array<SurfaceBuffer *, latte::MaxRenderTargets> mActiveColorBuffers;
   ScanBufferChain mTvScanBuffers;
   ScanBufferChain mDrcScanBuffers;

   gl::GLuint mFeedbackQuery = 0;
   uint32_t mFeedbackBaseOffset[latte::MaxStreamOutBuffers];
   uint32_t mFeedbackCurrentOffset[latte::MaxStreamOutBuffers];

   gl::GLuint mOccQuery = 0;
   uint32_t mLastOccQueryAddress = 0;

   latte::ContextState *mContextState = nullptr;

   pm4::EventWriteEOP mPendingEOP;

   std::array<ColorBufferCache, latte::MaxRenderTargets> mColorBufferCache;
   DepthBufferCache mDepthBufferCache;
   std::array<TextureCache, latte::MaxTextures> mPixelTextureCache;
   std::array<SamplerCache, latte::MaxSamplers> mPixelSamplerCache;
   std::array<FeedbackBufferCache, latte::MaxStreamOutBuffers> mFeedbackBufferCache;

   using duration_system_clock = std::chrono::duration<double, std::chrono::system_clock::period>;
   using duration_ms = std::chrono::duration<double, std::chrono::milliseconds::period>;
   std::chrono::time_point<std::chrono::system_clock> mLastSwap;
   duration_system_clock mAverageFrameTime;

#ifdef PLATFORM_WINDOWS
   uint64_t mDeviceContext = 0;
   uint64_t mOpenGLContext = 0;
#endif
};

} // namespace opengl

} // namespace gpu
