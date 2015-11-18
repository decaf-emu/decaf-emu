#pragma once
#include <array_view.h>
#include <glbinding/gl/types.h>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>
#include "gpu/pm4.h"
#include "gpu/driver.h"

namespace gpu
{

namespace opengl
{

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
      latte::SQ_FORMAT_COMP formatComp;
   };

   std::vector<Attrib> attribs;
   latte::SQ_PGM_START_FS pgm_start_fs;
   bool parsed = false;
};

struct VertexShader
{
   latte::SQ_PGM_START_VS pgm_start_vs;
   gl::GLuint program = 0;
   std::string code;
};

struct PixelShader
{
   latte::SQ_PGM_START_PS pgm_start_ps;
   gl::GLuint program = 0;
   std::string code;
};

using ShaderKey = std::tuple<uint32_t, uint32_t, uint32_t>;

struct Shader
{
   gl::GLuint pipeline = 0;
   FetchShader *fetch;
   VertexShader *vertex;
   PixelShader *pixel;
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
   void activateDeviceContext();

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

   bool checkActiveShader();

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
   uint32_t mRegisters[0x10000];
   std::thread mThread;

   Shader *mActiveShader = nullptr;
   std::unordered_map<uint32_t, FetchShader> mFetchShaders;
   std::unordered_map<uint32_t, VertexShader> mVertexShaders;
   std::unordered_map<uint32_t, PixelShader> mPixelShaders;
   std::map<ShaderKey, Shader> mShaders;

   GLContext mDeviceContext;
};

} // namespace opengl

} // namespace gpu
