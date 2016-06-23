#pragma once
#include "common/platform.h"
#include "common/log.h"
#include "gpu/pm4.h"
#include "gpu/latte_contextstate.h"
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

static const auto MAX_ATTRIB_COUNT = 16u;
static const auto MAX_COLOR_BUFFER_COUNT = 8u;
static const auto MAX_UNIFORM_BLOCKS = 8u;
static const auto MAX_UNIFORM_REGISTERS = 256u;
static const auto MAX_SAMPLERS_PER_TYPE = 16u;

class unimplemented_error : public std::exception
{
public:
   unimplemented_error(const std::string &message) :
      std::exception(),
      mWhat(message)
   {
      gLog->critical(message);
   }

   virtual char const* what() const noexcept
   {
      return mWhat.c_str();
   }

private:
   std::string mWhat;
};

enum class SamplerType
{
   Invalid,
   Sampler1D,
   Sampler2D,
   Sampler2DArray,
};

struct FetchShader
{
   struct Attrib
   {
      uint32_t buffer;
      uint32_t offset;
      uint32_t location;
      uint32_t bytesPerElement;
      latte::SQ_DATA_FORMAT format;
      latte::SQ_SEL dstSel[4];
      latte::SQ_NUM_FORMAT numFormat;
      latte::SQ_ENDIAN endianSwap;
      latte::SQ_FORMAT_COMP formatComp;
   };

   gl::GLuint object = 0;
   std::vector<Attrib> attribs;
   latte::SQ_PGM_START_FS pgm_start_fs;
};

struct VertexShader
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   std::array<gl::GLuint, MAX_ATTRIB_COUNT> attribLocations;
   std::array<SamplerType, MAX_SAMPLERS_PER_TYPE> samplerTypes;
   latte::SQ_PGM_START_VS pgm_start_vs;
   std::string code;
   std::string disassembly;
};

struct PixelShader
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   std::array<SamplerType, MAX_SAMPLERS_PER_TYPE> samplerTypes;
   latte::SQ_PGM_START_PS pgm_start_ps;
   std::string code;
   std::string disassembly;
};

using ShaderKey = std::tuple<uint32_t, uint32_t, uint32_t>;

struct Shader
{
   gl::GLuint object = 0;
   FetchShader *fetch;
   VertexShader *vertex;
   PixelShader *pixel;
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

struct SurfaceBuffer : Resource
{
   gl::GLuint object = 0;
   SurfaceUseState state = SurfaceUseState::None;
   bool dirtyAsTexture = true;
   uint32_t width = 0;
   uint32_t height = 0;
   uint32_t depth = 0;
};

struct ScanBufferChain
{
   gl::GLuint object = 0;
   uint32_t width;
   uint32_t height;
};

struct FrameBuffer
{
   gl::GLuint object = 0;
};

struct AttributeBuffer
{
   gl::GLuint object = 0;
   uint32_t size = 0;
   void *mappedBuffer = nullptr;
};

struct Texture
{
   gl::GLuint object = 0;
   uint32_t words[7];
};

struct Sampler
{
   gl::GLuint object = 0;
};

struct UniformBuffer
{
   gl::GLuint object = 0;
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
   virtual void setForcedGpuSync(bool enabled) override;

private:
   void initGL();

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
   void drawIndexAuto(const pm4::DrawIndexAuto &data);
   void drawIndex2(const pm4::DrawIndex2 &data);
   void indexType(const pm4::IndexType &data);
   void indirectBufferCall(const pm4::IndirectBufferCall &data);
   void numInstances(const pm4::NumInstances &data);
   void memWrite(const pm4::MemWrite &data);
   void eventWriteEOP(const pm4::EventWriteEOP &data);
   void handlePendingEOP();

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
   getSurfaceBuffer(ppcaddr_t baseAddress, uint32_t width, uint32_t height, uint32_t depth,
                    latte::SQ_TEX_DIM dim, latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT numFormat,
                    latte::SQ_FORMAT_COMP formatComp, uint32_t degamma);

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
   bool checkActiveSamplers();
   bool checkActiveShader();
   bool checkActiveTextures();
   bool checkActiveUniforms();
   bool checkViewport();

   void setRegister(latte::Register reg, uint32_t value);

   bool parseFetchShader(FetchShader &shader, void *buffer, size_t size);
   bool compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size);
   bool compilePixelShader(PixelShader &pixel, uint8_t *buffer, size_t size);

   void runCommandBuffer(uint32_t *buffer, uint32_t size);

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      static_assert(sizeof(Type) == 4, "Register storage must be a uint32_t");
      return *reinterpret_cast<Type *>(&mRegisters[id / 4]);
   }

private:
   volatile bool mRunning = true;
   std::thread mThread;
   bool mSyncEnabled;
   std::mutex mSyncLock;
   std::condition_variable mSyncCond;
   uint64_t mSyncFlipCount;

   std::array<uint32_t, 0x10000> mRegisters;

   bool mViewportDirty = false;
   bool mScissorDirty = false;

   std::unordered_map<uint32_t, FetchShader> mFetchShaders;
   std::unordered_map<uint32_t, VertexShader> mVertexShaders;
   std::unordered_map<uint32_t, PixelShader> mPixelShaders;
   std::map<ShaderKey, Shader> mShaders;
   std::unordered_map<uint64_t, SurfaceBuffer> mSurfaces;
   std::unordered_map<uint32_t, Texture> mTextures;
   std::unordered_map<uint32_t, AttributeBuffer> mAttribBuffers;
   std::unordered_map<uint32_t, UniformBuffer> mUniformBuffers;

   std::array<Sampler, MAX_SAMPLERS_PER_TYPE> mVertexSamplers;
   std::array<Sampler, MAX_SAMPLERS_PER_TYPE> mPixelSamplers;
   std::array<Sampler, MAX_SAMPLERS_PER_TYPE> mGeometrySamplers;

   FrameBuffer mFrameBuffer;
   Shader *mActiveShader = nullptr;
   SurfaceBuffer *mActiveDepthBuffer = nullptr;
   std::array<SurfaceBuffer *, MAX_COLOR_BUFFER_COUNT> mActiveColorBuffers;
   ScanBufferChain mTvScanBuffers;
   ScanBufferChain mDrcScanBuffers;

   latte::ContextState *mContextState = nullptr;

   pm4::EventWriteEOP mPendingEOP;

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
