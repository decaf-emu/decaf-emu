#pragma once
#include <array_view.h>
#include <glbinding/gl/types.h>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>
#include "gpu/pm4.h"
#include "gpu/driver.h"
#include "platform/platform.h"

namespace gpu
{

namespace opengl
{

static const auto MAX_ATTRIB_COUNT = 16u;
static const auto MAX_COLOR_BUFFER_COUNT = 8u;
static const auto MAX_UNIFORM_BLOCKS = 8u;
static const auto MAX_UNIFORM_REGISTERS = 256u;

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
   std::array<gl::GLuint, MAX_UNIFORM_BLOCKS> uniformBlocks;
   latte::SQ_PGM_START_VS pgm_start_vs;
   std::string code;
};

struct PixelShader
{
   gl::GLuint object = 0;
   gl::GLuint uniformRegisters = 0;
   std::array<gl::GLuint, MAX_UNIFORM_BLOCKS> uniformBlocks;
   latte::SQ_PGM_START_PS pgm_start_ps;
   std::string code;
};

using ShaderKey = std::tuple<uint32_t, uint32_t, uint32_t>;

struct Shader
{
   gl::GLuint object = 0;
   FetchShader *fetch;
   VertexShader *vertex;
   PixelShader *pixel;
};

struct ColorBuffer
{
   gl::GLuint object = 0;
   latte::CB_COLORN_BASE cb_color_base;
};

struct DepthBuffer
{
   gl::GLuint object = 0;
   latte::DB_DEPTH_BASE db_depth_base;
};

struct FrameBuffer
{
   gl::GLuint object = 0;
};

struct AttributeBuffer
{
   gl::GLuint object = 0;
   uint32_t size = 0;
   uint32_t stride = 0;
   void *mappedBuffer = nullptr;
};

struct Texture
{
   gl::GLuint object = 0;
};

struct ScreenDrawData
{
   gl::GLuint vertexProgram;
   gl::GLuint pixelProgram;
   gl::GLuint pipeline;
   gl::GLuint vertArray;
   gl::GLuint vertBuffer;
};

using GLContext = uint64_t;

class GLDriver : public gpu::Driver
{
public:
   virtual ~GLDriver() {}

   void start() override;
   void setupWindow() override;
   void setTvDisplay(size_t width, size_t height) override;
   void setDrcDisplay(size_t width, size_t height) override;

private:
   void run();
   void initGL();

   void activateDeviceContext();
   void swapBuffers();

   void handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data);
   void decafCopyColorToScan(pm4::DecafCopyColorToScan &data);
   void decafSwapBuffers(pm4::DecafSwapBuffers &data);
   void decafClearColor(pm4::DecafClearColor &data);
   void decafClearDepthStencil(pm4::DecafClearDepthStencil &data);
   void drawIndexAuto(pm4::DrawIndexAuto &data);
   void drawIndex2(pm4::DrawIndex2 &data);
   void indexType(pm4::IndexType &data);
   void numInstances(pm4::NumInstances &data);
   void setAluConsts(pm4::SetAluConsts &data);
   void setConfigRegs(pm4::SetConfigRegs &data);
   void setContextRegs(pm4::SetContextRegs &data);
   void setControlConstants(pm4::SetControlConstants &data);
   void setLoopConsts(pm4::SetLoopConsts &data);
   void setSamplers(pm4::SetSamplers &data);
   void setResources(pm4::SetResources &data);
   void indirectBufferCall(pm4::IndirectBufferCall &data);

   ColorBuffer * getColorBuffer(latte::CB_COLORN_BASE &base,
      latte::CB_COLORN_SIZE &size, latte::CB_COLORN_INFO &info);
   DepthBuffer * getDepthBuffer(latte::DB_DEPTH_BASE &db_depth_base,
      latte::DB_DEPTH_SIZE &db_depth_size, latte::DB_DEPTH_INFO &db_depth_info);

   bool checkReadyDraw();
   bool checkActiveAttribBuffers();
   bool checkActiveColorBuffer();
   bool checkActiveDepthBuffer();
   bool checkActiveShader();
   bool checkActiveTextures();
   bool checkActiveUniforms();
   bool checkViewport();

   void setRegister(latte::Register::Value reg, uint32_t value);

   bool parseFetchShader(FetchShader &shader, void *buffer, size_t size);
   bool compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size);
   bool compilePixelShader(PixelShader &pixel, uint8_t *buffer, size_t size);

   void runCommandBuffer(uint32_t *buffer, uint32_t size);

   template<typename Type>
   Type getRegister(uint32_t id)
   {
      return Type { mRegisters[id / 4] };
   }

private:
   volatile bool mRunning = true;
   std::array<uint32_t, 0x10000> mRegisters;
   std::thread mThread;

   bool mViewportDirty = false;
   bool mScissorDirty = false;

   Shader *mActiveShader = nullptr;
   std::unordered_map<uint32_t, FetchShader> mFetchShaders;
   std::unordered_map<uint32_t, VertexShader> mVertexShaders;
   std::unordered_map<uint32_t, PixelShader> mPixelShaders;
   std::map<ShaderKey, Shader> mShaders;
   std::unordered_map<uint32_t, Texture> mTextures;

   ScreenDrawData mScreenDraw;
   std::unordered_map<uint32_t, ColorBuffer> mColorBuffers;
   std::unordered_map<uint32_t, DepthBuffer> mDepthBuffers;
   std::unordered_map<uint32_t, AttributeBuffer> mAttribBuffers;

   FrameBuffer mFrameBuffer;
   DepthBuffer *mActiveDepthBuffer;
   std::array<ColorBuffer *, MAX_COLOR_BUFFER_COUNT> mActiveColorBuffers;

#ifdef PLATFORM_WINDOWS
   uint64_t mDeviceContext;
   uint64_t mOpenGLContext;
#endif
};

} // namespace opengl

} // namespace gpu
