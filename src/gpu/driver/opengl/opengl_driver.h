#pragma once
#include <array_view.h>
#include <glbinding/gl/types.h>
#include <map>
#include <thread>
#include <unordered_map>
#include <vector>
#include "gpu/pm4.h"

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
   gl::GLuint program = -1;
};

struct PixelShader
{
   latte::SQ_PGM_START_PS pgm_start_ps;
   gl::GLuint program = -1;
};

using ShaderKey = std::pair<uint32_t, uint32_t>;

struct Shader
{
   gl::GLuint program = -1;
   FetchShader *fetch;
   VertexShader *vertex;
   PixelShader *pixel;
};

class Driver
{
public:
   void start();
   void run();

private:
   void handlePacketType3(pm4::Packet3 header, gsl::array_view<uint32_t> data);
   void setContextReg(pm4::SetContextRegs &data);

   void checkActiveShader();

   void setRegister(latte::Register::Value reg, uint32_t value);

   bool parseFetchShader(FetchShader &shader, void *buffer, size_t size);
   bool compileVertexShader(VertexShader &vertex, FetchShader &fetch, void *buffer, size_t size);

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
};

} // namespace opengl

} // namespace gpu
