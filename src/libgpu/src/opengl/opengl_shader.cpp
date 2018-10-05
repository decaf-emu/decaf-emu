#ifdef DECAF_GL
#include "glsl2/glsl2_translate.h"
#include "gpu_config.h"
#include "gpu_memory.h"
#include "latte/latte_formats.h"
#include "latte/latte_registers.h"
#include "latte/latte_disassembler.h"
#include "opengl_constants.h"
#include "opengl_driver.h"

#include <common/decaf_assert.h>
#include <common/log.h>
#include <common/murmur3.h>
#include <common/platform_dir.h>
#include <common/strutils.h>
#include <fmt/format.h>
#include <fstream>
#include <glbinding/gl/gl.h>
#include <libcpu/mem.h>

namespace opengl
{

// TODO: This is really some kind of 'nsight compat mode'...  Maybe
//  it should even be a configurable option (related to force_sync).
static const auto USE_PERSISTENT_MAP = true;

// This represents the number of bytes of padding to attach to the end
//  of an attribute buffer.  This is neccessary for cases where the game
//  fetches past the edge of a buffer, but does not use it.
static const auto BUFFER_PADDING = 16;

// Enable workaround for NVIDIA GLSL compiler bug which incorrectly fails
//  on "layout(xfb_buffer = A, xfb_stride = B)" syntax when some buffers
//  have different strides than others.
static const auto NVIDIA_GLSL_WORKAROUND = true;


static void
dumpRawShader(const std::string &type,
              phys_addr address,
              uint32_t size,
              bool isSubroutine = false)
{
   if (!gpu::config::dump_shaders) {
      return;
   }

   auto filePath = fmt::format("dump/gpu_{}_{}.txt", type, address);
   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   // Disassemble
   auto file = std::ofstream { filePath, std::ofstream::out };
   auto binary = gsl::make_span(gpu::internal::translateAddress<uint8_t>(address), size);
   auto output = latte::disassemble(binary, isSubroutine);
   file << output << std::endl;
}

static void
dumpTranslatedShader(const std::string &type,
                     phys_addr address,
                     const std::string &shaderSource)
{
   if (!gpu::config::dump_shaders) {
      return;
   }

   auto filePath = fmt::format("dump/gpu_{}_{}.glsl", type, address);
   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   auto file = std::ofstream { filePath, std::ofstream::out };
   file << shaderSource << std::endl;
}

static gl::GLenum
getDataFormatGlType(latte::SQ_DATA_FORMAT format)
{
   // Some Special Cases
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
      return gl::GL_UNSIGNED_INT;
   }

   auto bitCount = latte::getDataFormatComponentBits(format);

   switch (bitCount) {
   case 8:
      return gl::GL_UNSIGNED_BYTE;
   case 16:
      return gl::GL_UNSIGNED_SHORT;
   case 32:
      return gl::GL_UNSIGNED_INT;
   default:
      decaf_abort(fmt::format("Unimplemented attribute bit count: {} for {}", bitCount, format));
   }
}

static void
deleteShaderObject(FetchShader *shader)
{
   gl::glDeleteVertexArrays(1, &shader->object);
}

static void
deleteShaderObject(VertexShader *shader)
{
   gl::glDeleteProgram(shader->object);
}

static void
deleteShaderObject(PixelShader *shader)
{
   gl::glDeleteProgram(shader->object);
}

template <typename ShaderPtrType> static bool
invalidateShaderIfChanged(ShaderPtrType &shader,
                          uint64_t shaderKey,
                          std::unordered_map<uint64_t, ShaderPtrType> &shaders,
                          ResourceMemoryMap &resourceMap)
{
   if (!shader || !shader->needRebuild) {
      return false;
   }

   // Check whether the shader has actually changed; we want to avoid
   //  recompiling shaders if possible, since that's very slow.
   //  Note that we don't save this, which means we have to compute it
   //  twice if we end up recreating the shader, but that cost is tiny
   //  compared to the time it takes to actually create the shader.
   uint64_t newHash[2] = { 0, 0 };
   MurmurHash3_x64_128(gpu::internal::translateAddress(shader->cpuMemStart),
                       static_cast<int>(shader->cpuMemEnd - shader->cpuMemStart),
                       0, newHash);
   if (newHash[0] == shader->cpuMemHash[0] && newHash[1] == shader->cpuMemHash[1]) {
      shader->needRebuild = false;
      return false;
   }

   // The shader contents have changed, so delete the existing object
   deleteShaderObject(shader);
   shader->object = 0;

   shader->refCount--;
   if (shader->refCount == 0) {
      resourceMap.removeResource(shader);
      delete shader;
   }
   shader = nullptr;

   // Also clear out the table entry (but leave it allocated since we're
   //  about to reuse it).  Do this last in case shader is referencing the
   //  entry itself.
   shaders[shaderKey] = nullptr;

   return true;
}

bool GLDriver::checkActiveShader()
{
   auto pgm_start_fs = getRegister<latte::SQ_PGM_START_FS>(latte::Register::SQ_PGM_START_FS);
   auto pgm_start_vs = getRegister<latte::SQ_PGM_START_VS>(latte::Register::SQ_PGM_START_VS);
   auto pgm_start_ps = getRegister<latte::SQ_PGM_START_PS>(latte::Register::SQ_PGM_START_PS);
   auto pgm_size_fs = getRegister<latte::SQ_PGM_SIZE_FS>(latte::Register::SQ_PGM_SIZE_FS);
   auto pgm_size_vs = getRegister<latte::SQ_PGM_SIZE_VS>(latte::Register::SQ_PGM_SIZE_VS);
   auto pgm_size_ps = getRegister<latte::SQ_PGM_SIZE_PS>(latte::Register::SQ_PGM_SIZE_PS);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto db_shader_control = getRegister<latte::DB_SHADER_CONTROL>(latte::Register::DB_SHADER_CONTROL);
   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);
   auto sx_alpha_ref = getRegister<latte::SX_ALPHA_REF>(latte::Register::SX_ALPHA_REF);
   auto vgt_strmout_en = getRegister<latte::VGT_STRMOUT_EN>(latte::Register::VGT_STRMOUT_EN);
   auto pa_cl_clip_cntl = getRegister<latte::PA_CL_CLIP_CNTL>(latte::Register::PA_CL_CLIP_CNTL);
   auto vgt_primitive_type = getRegister<latte::VGT_PRIMITIVE_TYPE>(latte::Register::VGT_PRIMITIVE_TYPE);
   auto isScreenSpace = (vgt_primitive_type.PRIM_TYPE() == latte::VGT_DI_PRIMITIVE_TYPE::RECTLIST);

   if (!pgm_start_fs.PGM_START()) {
      gLog->error("Fetch shader was not set");
      return false;
   }

   if (!pgm_start_vs.PGM_START()) {
      gLog->error("Vertex shader was not set");
      return false;
   }

   if (!vgt_strmout_en.STREAMOUT() && !pgm_start_ps.PGM_START()) {
      gLog->error("Pixel shader was not set (and transform feedback was not enabled)");
      return false;
   }

   auto fsPgmAddress = phys_addr { pgm_start_fs.PGM_START() << 8 };
   auto vsPgmAddress = phys_addr { pgm_start_vs.PGM_START() << 8 };
   auto psPgmAddress = phys_addr { pgm_start_ps.PGM_START() << 8 };
   auto fsPgmSize = pgm_size_fs.PGM_SIZE() << 3;
   auto vsPgmSize = pgm_size_vs.PGM_SIZE() << 3;
   auto psPgmSize = pgm_size_ps.PGM_SIZE() << 3;
   auto z_order = db_shader_control.Z_ORDER();
   auto early_z = (z_order == latte::DB_Z_ORDER::EARLY_Z_THEN_LATE_Z || z_order == latte::DB_Z_ORDER::EARLY_Z_THEN_RE_Z);
   auto alphaTestFunc = sx_alpha_test_control.ALPHA_FUNC();

   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE() || sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      alphaTestFunc = latte::REF_FUNC::ALWAYS;
   }

   // We don't currently handle offsets, so panic if any are nonzero
   decaf_check(getRegister<uint32_t>(latte::Register::SQ_PGM_CF_OFFSET_PS) == 0);
   decaf_check(getRegister<uint32_t>(latte::Register::SQ_PGM_CF_OFFSET_VS) == 0);
   decaf_check(getRegister<uint32_t>(latte::Register::SQ_PGM_CF_OFFSET_GS) == 0);
   decaf_check(getRegister<uint32_t>(latte::Register::SQ_PGM_CF_OFFSET_ES) == 0);
   decaf_check(getRegister<uint32_t>(latte::Register::SQ_PGM_CF_OFFSET_FS) == 0);

   // Make FS key
   auto fsShaderKey = static_cast<uint64_t>(fsPgmAddress) << 32;

   // Make VS key
   auto vsShaderKey = static_cast<uint64_t>(vsPgmAddress) << 32;
   vsShaderKey ^= (isScreenSpace ? 1ull : 0ull) << 31;

   if (vgt_strmout_en.STREAMOUT()) {
      vsShaderKey |= 1;

      for (auto i = 0u; i < latte::MaxStreamOutBuffers; ++i) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
         vsShaderKey ^= vgt_strmout_vtx_stride << (1ull + 7 * i);
      }
   }

   // Make PS key
   uint64_t psShaderKey;

   if (pa_cl_clip_cntl.RASTERISER_DISABLE()) {
      psShaderKey = 0;
   } else {
      psShaderKey = static_cast<uint64_t>(psPgmAddress) << 32;
      psShaderKey |= static_cast<uint64_t>(alphaTestFunc) << 28;
      psShaderKey |= static_cast<uint64_t>(early_z) << 27;
      psShaderKey |= static_cast<uint64_t>(db_shader_control.Z_EXPORT_ENABLE()) << 26;
      psShaderKey |= cb_shader_mask.value & 0xFF;
   }

   if (mActiveShader
    && mActiveShader->fetch && !mActiveShader->fetch->needRebuild && mActiveShader->fetchKey == fsShaderKey
    && mActiveShader->vertex && !mActiveShader->vertex->needRebuild && mActiveShader->vertexKey == vsShaderKey
    && (!mActiveShader->pixel || !mActiveShader->pixel->needRebuild) && mActiveShader->pixelKey == psShaderKey) {
      // We already have the current shader bound, nothing special to do.
      return true;
   }

   auto shaderKey = ShaderPipelineKey { fsShaderKey, vsShaderKey, psShaderKey };
   auto &pipeline = mShaderPipelines[shaderKey];

   // If the pipeline already exists, check whether any of its component
   //  shaders need to be rebuilt
   if (pipeline.object) {
      if (invalidateShaderIfChanged(pipeline.fetch, fsShaderKey, mFetchShaders, mResourceMap)
       || invalidateShaderIfChanged(pipeline.vertex, vsShaderKey, mVertexShaders, mResourceMap)
       || invalidateShaderIfChanged(pipeline.pixel, psShaderKey, mPixelShaders, mResourceMap)) {
         gl::glDeleteProgramPipelines(1, &pipeline.object);
         pipeline.object = 0;
      }
   }

   auto getProgramLog = [](auto program) {
      gl::GLint logLength = 0;
      std::string logMessage;
      gl::glGetProgramiv(program, gl::GL_INFO_LOG_LENGTH, &logLength);

      logMessage.resize(logLength);
      gl::glGetProgramInfoLog(program, logLength, &logLength, &logMessage[0]);
      return logMessage;
   };

   // Generate shader if needed
   if (!pipeline.object) {
      // Parse fetch shader if needed
      auto &fetchShader = mFetchShaders[fsShaderKey];
      invalidateShaderIfChanged(fetchShader, fsShaderKey, mFetchShaders, mResourceMap);

      if (!fetchShader) {
         auto aluDivisor0 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_0);
         auto aluDivisor1 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_1);

         fetchShader = new FetchShader {};
         fetchShader->cpuMemStart = fsPgmAddress;
         fetchShader->cpuMemEnd = fsPgmAddress + fsPgmSize;
         MurmurHash3_x64_128(gpu::internal::translateAddress(fetchShader->cpuMemStart),
                             static_cast<int>(fetchShader->cpuMemEnd - fetchShader->cpuMemStart),
                             0, fetchShader->cpuMemHash);
         fetchShader->dirtyMemory = false;
         mResourceMap.addResource(fetchShader);

         dumpRawShader("fetch", fsPgmAddress, fsPgmSize, true);
         fetchShader->disassembly = latte::disassemble(gsl::make_span(gpu::internal::translateAddress<uint8_t>(fsPgmAddress), fsPgmSize), true);

         if (!parseFetchShader(*fetchShader, gpu::internal::translateAddress(fsPgmAddress), fsPgmSize)) {
            gLog->error("Failed to parse fetch shader");
            return false;
         }

         // Setup attrib format
         gl::glCreateVertexArrays(1, &fetchShader->object);
         if (gpu::config::debug) {
            auto label = fmt::format("fetch shader @ {}", fsPgmAddress);
            gl::glObjectLabel(gl::GL_VERTEX_ARRAY, fetchShader->object, -1, label.c_str());
         }

         auto bufferUsed = std::array<bool, latte::MaxAttribBuffers> { false };
         auto bufferDivisor = std::array<uint32_t, latte::MaxAttribBuffers> { 0 };

         for (auto &attrib : fetchShader->attribs) {
            auto resourceId = attrib.buffer + latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0;

            if (resourceId >= latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 && resourceId < latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + 0x10) {
               auto attribBufferId = resourceId - latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0;
               auto type = getDataFormatGlType(attrib.format);
               auto components = latte::getDataFormatComponents(attrib.format);
               auto divisor = 0u;

               gl::glEnableVertexArrayAttrib(fetchShader->object, attrib.location);
               gl::glVertexArrayAttribIFormat(fetchShader->object, attrib.location, components, type, attrib.offset);
               gl::glVertexArrayAttribBinding(fetchShader->object, attrib.location, attribBufferId);

               if (attrib.type == latte::SQ_VTX_FETCH_TYPE::INSTANCE_DATA) {
                  if (attrib.srcSelX == latte::SQ_SEL::SEL_W) {
                     divisor = 1;
                  } else if (attrib.srcSelX == latte::SQ_SEL::SEL_Y) {
                     divisor = aluDivisor0;
                  } else if (attrib.srcSelX == latte::SQ_SEL::SEL_Z) {
                     divisor = aluDivisor1;
                  } else {
                     decaf_abort(fmt::format("Unexpected SRC_SEL_X {} for alu divisor", attrib.srcSelX));
                  }
               }

               decaf_assert(!bufferUsed[attribBufferId] || bufferDivisor[attribBufferId] == divisor,
                  "Multiple attributes conflict on buffer divisor mode");

               bufferUsed[attribBufferId] = true;
               bufferDivisor[attribBufferId] = divisor;
            } else {
               decaf_abort("We do not yet support binding of non-attribute buffers");
            }
         }

         for (auto bufferId = 0; bufferId < latte::MaxAttribBuffers; ++bufferId) {
            if (bufferUsed[bufferId]) {
               gl::glVertexArrayBindingDivisor(fetchShader->object, bufferId, bufferDivisor[bufferId]);
            }
         }
      }

      pipeline.fetch = fetchShader;
      pipeline.fetch->refCount++;
      pipeline.fetchKey = fsShaderKey;

      // Compile vertex shader if needed
      auto &vertexShader = mVertexShaders[vsShaderKey];
      invalidateShaderIfChanged(vertexShader, vsShaderKey, mVertexShaders, mResourceMap);

      if (!vertexShader) {
         vertexShader = new VertexShader;

         vertexShader->cpuMemStart = vsPgmAddress;
         vertexShader->cpuMemEnd = vsPgmAddress + vsPgmSize;
         MurmurHash3_x64_128(gpu::internal::translateAddress(vertexShader->cpuMemStart),
                             static_cast<int>(vertexShader->cpuMemEnd - vertexShader->cpuMemStart),
                             0, vertexShader->cpuMemHash);
         vertexShader->dirtyMemory = false;
         mResourceMap.addResource(vertexShader);

         dumpRawShader("vertex", vsPgmAddress, vsPgmSize);

         if (!compileVertexShader(*vertexShader, *fetchShader, gpu::internal::translateAddress<uint8_t>(vsPgmAddress), vsPgmSize, isScreenSpace)) {
            gLog->error("Failed to recompile vertex shader");
            return false;
         }

         dumpTranslatedShader("vertex", vsPgmAddress, vertexShader->code);

         // Create OpenGL Shader
         const gl::GLchar *code[] = { vertexShader->code.c_str() };
         vertexShader->object = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, code);
         if (gpu::config::debug) {
            std::string label = fmt::format("vertex shader @ {}", vsPgmAddress);
            gl::glObjectLabel(gl::GL_PROGRAM, vertexShader->object, -1, label.c_str());
         }

         // Check if shader compiled & linked properly
         gl::GLint isLinked = 0;
         gl::glGetProgramiv(vertexShader->object, gl::GL_LINK_STATUS, &isLinked);

         if (!isLinked) {
            auto log = getProgramLog(vertexShader->object);
            gLog->error("OpenGL failed to compile vertex shader:\n{}", log);
            gLog->error("Fetch Disassembly:\n{}\n", fetchShader->disassembly);
            gLog->error("Shader Disassembly:\n{}\n", vertexShader->disassembly);
            gLog->error("Shader Code:\n{}\n", vertexShader->code);
            return false;
         }

         // Get uniform locations
         vertexShader->uniformRegisters = gl::glGetUniformLocation(vertexShader->object, "VR");
         vertexShader->uniformViewport = gl::glGetUniformLocation(vertexShader->object, "uViewport");

         // Get attribute locations
         vertexShader->attribLocations.fill(0);

         for (auto &attrib : fetchShader->attribs) {
            auto name = fmt::format("fs_out_{}", attrib.location);
            vertexShader->attribLocations[attrib.location] = gl::glGetAttribLocation(vertexShader->object, name.c_str());
         }
      }

      pipeline.vertex = vertexShader;
      pipeline.vertex->refCount++;
      pipeline.vertexKey = vsShaderKey;

      if (pa_cl_clip_cntl.RASTERISER_DISABLE()) {

         // Rasterization disabled; no pixel shader used
         pipeline.pixel = nullptr;

      } else {

         // Transform feedback disabled; compile pixel shader if needed
         auto &pixelShader = mPixelShaders[psShaderKey];
         invalidateShaderIfChanged(pixelShader, psShaderKey, mPixelShaders, mResourceMap);

         if (!pixelShader) {
            pixelShader = new PixelShader;

            pixelShader->cpuMemStart = psPgmAddress;
            pixelShader->cpuMemEnd = psPgmAddress + psPgmSize;
            MurmurHash3_x64_128(gpu::internal::translateAddress(pixelShader->cpuMemStart),
                                static_cast<int>(pixelShader->cpuMemEnd - pixelShader->cpuMemStart),
                                0, pixelShader->cpuMemHash);
            pixelShader->dirtyMemory = false;
            mResourceMap.addResource(pixelShader);

            dumpRawShader("pixel", psPgmAddress, psPgmSize);

            if (!compilePixelShader(*pixelShader, *vertexShader, gpu::internal::translateAddress<uint8_t>(psPgmAddress), psPgmSize)) {
               gLog->error("Failed to recompile pixel shader");
               return false;
            }

            dumpTranslatedShader("pixel", psPgmAddress, pixelShader->code);

            // Create OpenGL Shader
            const gl::GLchar *code[] = { pixelShader->code.c_str() };
            pixelShader->object = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, code);

            if (gpu::config::debug) {
               std::string label = fmt::format("pixel shader @ {}", psPgmAddress);
               gl::glObjectLabel(gl::GL_PROGRAM, pixelShader->object, -1, label.c_str());
            }

            // Check if shader compiled & linked properly
            gl::GLint isLinked = 0;
            gl::glGetProgramiv(pixelShader->object, gl::GL_LINK_STATUS, &isLinked);

            if (!isLinked) {
               auto log = getProgramLog(pixelShader->object);
               gLog->error("OpenGL failed to compile pixel shader:\n{}", log);
               gLog->error("Shader Disassembly:\n{}\n", pixelShader->disassembly);
               gLog->error("Shader Code:\n{}\n", pixelShader->code);
               return false;
            }

            // Get uniform locations
            pixelShader->uniformRegisters = gl::glGetUniformLocation(pixelShader->object, "PR");
            pixelShader->uniformAlphaRef = gl::glGetUniformLocation(pixelShader->object, "uAlphaRef");
            pixelShader->sx_alpha_test_control = sx_alpha_test_control;
         }

         pipeline.pixel = pixelShader;
         pipeline.pixel->refCount++;
      }

      pipeline.pixelKey = psShaderKey;

      // Create pipeline
      gl::glCreateProgramPipelines(1, &pipeline.object);
      if (gpu::config::debug) {
         std::string label;

         if (pipeline.pixel) {
            label = fmt::format("shader set: fs = {}, vs = {}, ps = {}", fsPgmAddress, vsPgmAddress, psPgmAddress);
         } else {
            label = fmt::format("shader set: fs = {}, vs = {}, ps = none", fsPgmAddress, vsPgmAddress);
         }

         gl::glObjectLabel(gl::GL_PROGRAM_PIPELINE, pipeline.object, -1, label.c_str());
      }

      gl::glUseProgramStages(pipeline.object, gl::GL_VERTEX_SHADER_BIT, pipeline.vertex->object);
      gl::glUseProgramStages(pipeline.object, gl::GL_FRAGMENT_SHADER_BIT, pipeline.pixel ? pipeline.pixel->object : 0);
   }

   // Set active shader
   mActiveShader = &pipeline;

   // Set alpha reference
   if (mActiveShader->pixel && alphaTestFunc != latte::REF_FUNC::ALWAYS && alphaTestFunc != latte::REF_FUNC::NEVER) {
      gl::glProgramUniform1f(mActiveShader->pixel->object, mActiveShader->pixel->uniformAlphaRef, sx_alpha_ref.ALPHA_REF());
   }

   // Bind fetch shader
   gl::glBindVertexArray(pipeline.fetch->object);

   // Bind vertex + pixel shader
   gl::glBindProgramPipeline(pipeline.object);
   return true;
}

int
GLDriver::countModifiedUniforms(latte::Register firstReg,
                                uint32_t lastUniformUpdate)
{
   auto firstUniform = (firstReg - latte::Register::AluConstRegisterBase) / (4 * 4);
   decaf_check(firstUniform == 0 || firstUniform == 256);

   // We track uniforms in groups of 16 vectors, as a balance between the
   //  cost of uploading many unchanged uniforms and the cost of checking
   //  many individual uniforms for changes.
   auto offset = firstUniform / 16;

   for (auto i = 16u; i > 0; --i) {
      // lastUniformUpdate is set to the newly incremented uniform update
      //  generation at the time of the last upload, so any uniforms whose
      //  update generation is at least that value need to be uploaded.
      //  Need to be careful with this comparison in case of wraparound!
      auto diff = mLastUniformUpdate[offset + (i-1)] - lastUniformUpdate;

      if (diff <= 0x7FFFFFFF) {  // i.e. signed(diff) >= 0
         return i * 16;
      }
   }

   return 0;
}

bool GLDriver::checkActiveUniforms()
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);

   if (!mActiveShader) {
      return true;
   }

   if (sq_config.DX9_CONSTS()) {
      // Upload uniform registers
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         auto uploadCount = countModifiedUniforms(latte::Register::SQ_ALU_CONSTANT0_256, mActiveShader->vertex->lastUniformUpdate);

         if (uploadCount > 0) {
            auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_256 / 4]);
            gl::glProgramUniform4fv(mActiveShader->vertex->object, mActiveShader->vertex->uniformRegisters, uploadCount, values);

            mActiveShader->vertex->lastUniformUpdate = ++mUniformUpdateGen;
         }
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         auto uploadCount = countModifiedUniforms(latte::Register::SQ_ALU_CONSTANT0_0, mActiveShader->pixel->lastUniformUpdate);

         if (uploadCount > 0) {
            auto values = reinterpret_cast<float *>(&mRegisters[latte::Register::SQ_ALU_CONSTANT0_0 / 4]);
            gl::glProgramUniform4fv(mActiveShader->pixel->object, mActiveShader->pixel->uniformRegisters, uploadCount, values);

            mActiveShader->pixel->lastUniformUpdate = ++mUniformUpdateGen;
         }
      }
   } else {
      if (mActiveShader->vertex && mActiveShader->vertex->object) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            auto sq_alu_const_cache_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_VS_0 + 4 * i);
            auto sq_alu_const_buffer_size_vs = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_VS_0 + 4 * i);
            auto used = mActiveShader->vertex->usedUniformBlocks[i];
            auto bufferObject = gl::GLuint { 0 };

            if (!used || !sq_alu_const_buffer_size_vs) {
               bufferObject = 0;
            } else {
               auto addr = phys_addr { sq_alu_const_cache_vs << 8 };
               auto size = sq_alu_const_buffer_size_vs << 8;

               // Check that we can fit the uniform block into OpenGL buffers
               decaf_assert(size <= opengl::MaxUniformBlockSize,
                  fmt::format("Active uniform block with data size {} greater than what OpenGL supports {}", size, opengl::MaxUniformBlockSize));

               auto buffer = getDataBuffer(addr, size, true, false);
               bufferObject = buffer->object;
            }

            // Bind (or unbind) block
            if (mUniformBlockCache[i].vsObject != bufferObject) {
               mUniformBlockCache[i].vsObject = bufferObject;
               gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, i, bufferObject);
            }
         }
      }

      if (mActiveShader->pixel && mActiveShader->pixel->object) {
         for (auto i = 0u; i < latte::MaxUniformBlocks; ++i) {
            auto sq_alu_const_cache_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_CACHE_PS_0 + 4 * i);
            auto sq_alu_const_buffer_size_ps = getRegister<uint32_t>(latte::Register::SQ_ALU_CONST_BUFFER_SIZE_PS_0 + 4 * i);
            auto used = mActiveShader->pixel->usedUniformBlocks[i];
            auto bufferObject = gl::GLuint { 0 };

            if (!used || !sq_alu_const_buffer_size_ps) {
               bufferObject = 0;
            } else {
               auto addr = phys_addr { sq_alu_const_cache_ps << 8 };
               auto size = sq_alu_const_buffer_size_ps << 8;

               auto buffer = getDataBuffer(addr, size, true, false);
               bufferObject = buffer->object;
            }

            // Bind (or unbind) block
            if (mUniformBlockCache[i].psObject != bufferObject) {
               mUniformBlockCache[i].psObject = bufferObject;
               gl::glBindBufferBase(gl::GL_UNIFORM_BUFFER, 16 + i, bufferObject);
            }
         }
      }
   }

   return true;
}

DataBuffer *
GLDriver::getDataBuffer(phys_addr address,
                        size_t size,
                        bool isInput,
                        bool isOutput)
{
   decaf_check(size);
   decaf_check(isInput || isOutput);

   DataBuffer *buffer = &mDataBuffers[static_cast<uint32_t>(address)];

   if (buffer->object
    && buffer->allocatedSize >= size
    && !(isInput && !buffer->isInput)
    && !(isOutput && !buffer->isOutput)) {
      return buffer;  // No changes needed
   }

   auto oldObject = buffer->object;
   auto oldSize = buffer->allocatedSize;
   auto oldMappedBuffer = buffer->mappedBuffer;

   if (oldObject && (address != buffer->cpuMemStart || address + size != buffer->cpuMemEnd)) {
      if (buffer->isOutput) {
         mOutputBufferMap.removeResource(buffer);
      }
      mResourceMap.removeResource(buffer);
   }

   buffer->cpuMemStart = address;
   buffer->cpuMemEnd = address + size;
   buffer->allocatedSize = size;
   buffer->mappedBuffer = nullptr;
   buffer->isInput |= isInput;
   buffer->isOutput |= isOutput;

   // Never map output (transform feedback) buffers, because the only way
   //  to safely read data from them is by first calling glFinish().  With
   //  a non-mapped buffer, when we call glGet[Named]BufferSubData(), the
   //  driver can potentially wait just until that buffer is up to date
   //  without having to block on all other commands.
   auto shouldMap = USE_PERSISTENT_MAP && !buffer->isOutput;
   gl::glCreateBuffers(1, &buffer->object);

   if (gpu::config::debug) {
      const char *type = "output-only";

      if (buffer->isInput) {
         type = buffer->isOutput ? "input/output" : "input-only";
      }

      auto label = fmt::format("{} buffer @ {}", type, address);
      gl::glObjectLabel(gl::GL_BUFFER, buffer->object, -1, label.c_str());
   }

   auto usage = gl::BufferStorageMask::GL_NONE_BIT;

   if (buffer->isInput) {
      if (shouldMap) {
         usage |= gl::GL_MAP_WRITE_BIT;
      } else {
         usage |= gl::GL_DYNAMIC_STORAGE_BIT;
      }
   }

   if (buffer->isOutput) {
      usage |= gl::GL_CLIENT_STORAGE_BIT;
   }

   if (shouldMap) {
      usage |= gl::GL_MAP_PERSISTENT_BIT;
   }

   gl::glNamedBufferStorage(buffer->object, size + BUFFER_PADDING, nullptr, usage);

   if (oldObject) {
      if (oldMappedBuffer) {
         gl::glUnmapNamedBuffer(oldObject);
         buffer->mappedBuffer = nullptr;
      }

      gl::glCopyNamedBufferSubData(oldObject, buffer->object, 0, 0, std::min(oldSize, size));
      gl::glDeleteBuffers(1, &oldObject);
   }

   if (shouldMap) {
      auto access = gl::GL_MAP_PERSISTENT_BIT;

      if (buffer->isInput) {
         access |= gl::GL_MAP_WRITE_BIT | gl::GL_MAP_FLUSH_EXPLICIT_BIT;
      }

      if (buffer->isOutput) {
         access |= gl::GL_MAP_READ_BIT;
      }

      buffer->mappedBuffer = gl::glMapNamedBufferRange(buffer->object, 0, size + BUFFER_PADDING, access);
   }

   // Unconditionally upload the initial data store for attribute and
   //  uniform buffers.  This has to be done after mapping, since if we're
   //  using maps, we can't update the buffer via glBufferSubData (because
   //  we didn't specify GL_DYNAMIC_STORAGE_BIT).
   if (isInput) {
      if (!oldObject) {
         uploadDataBuffer(buffer, 0, size);
      } else if (size > oldSize) {
         uploadDataBuffer(buffer, oldSize, size - oldSize);
      }
   }

   mResourceMap.addResource(buffer);
   if (buffer->isOutput) {
      mOutputBufferMap.addResource(buffer);
   }

   return buffer;
}

void
GLDriver::downloadDataBuffer(DataBuffer *buffer,
                             size_t offset,
                             size_t size)
{
   if (buffer->mappedBuffer) {
      // We only map input-only buffers (see getDataBuffer()), so there's
      //  no need for a memory barrier here.
      decaf_check(!buffer->isOutput);

      memcpy(gpu::internal::translateAddress<char>(buffer->cpuMemStart) + offset,
             static_cast<char *>(buffer->mappedBuffer) + offset,
             size);
   } else {
      gl::glGetNamedBufferSubData(buffer->object, offset, size,
                                  gpu::internal::translateAddress<char>(buffer->cpuMemStart) + offset);
   }
}

void
GLDriver::uploadDataBuffer(DataBuffer *buffer,
                           size_t offset,
                           size_t size)
{
   // Avoid uploading the data if it hasn't changed.
   uint64_t newHash[2] = { 0, 0 };
   MurmurHash3_x64_128(gpu::internal::translateAddress(buffer->cpuMemStart),
                       static_cast<int>(buffer->allocatedSize),
                       0, newHash);

   if (newHash[0] != buffer->cpuMemHash[0] || newHash[1] != buffer->cpuMemHash[1]) {
      buffer->cpuMemHash[0] = newHash[0];
      buffer->cpuMemHash[1] = newHash[1];

      // We currently can't detect where the change occurred, so upload
      //  the entire buffer.  If we don't do this, the following sequence
      //  will result in incorrect GPU-side data:
      //     1) Client modifies two disjoint regions A and B of the buffer.
      //     2) Client calls GX2Invalidate() on region A.
      //     3) We detect that the hash has changed and upload region A.
      //     4) Client calls GX2Invalidate() on region B.
      //     5) We detect that the hash is unchanged and don't upload
      //         region B.
      //  Now region B has incorrect data on the host GPU.
      // TODO: Find a better way to allow partial updates.
      offset = 0;
      size = buffer->allocatedSize;

      if (buffer->mappedBuffer) {
         memcpy(static_cast<char *>(buffer->mappedBuffer) + offset,
                gpu::internal::translateAddress<char>(buffer->cpuMemStart) + offset,
                size);
         gl::glFlushMappedNamedBufferRange(buffer->object, offset, size);
         buffer->dirtyMap = true;
      } else {
         gl::glNamedBufferSubData(buffer->object, offset, size,
                                  gpu::internal::translateAddress<char>(buffer->cpuMemStart) + offset);
      }
   }
}

bool
GLDriver::checkActiveAttribBuffers()
{
   if (!mActiveShader
    || !mActiveShader->fetch || !mActiveShader->fetch->object
    || !mActiveShader->vertex || !mActiveShader->vertex->object) {
      return false;
   }

   auto needMemoryBarrier = false;

   for (auto i = 0u; i < latte::MaxAttribBuffers; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_ATTRIB_RESOURCE_0 + i) * 7;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_RESOURCE_WORD1_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_RESOURCE_WORD2_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_RESOURCE_WORD6_0 + 4 * resourceOffset);

      auto addr = phys_addr { sq_vtx_constant_word0.BASE_ADDRESS() };
      auto size = sq_vtx_constant_word1.SIZE() + 1;
      auto stride = sq_vtx_constant_word2.STRIDE();
      auto bufferObject = gl::GLuint { 0 };

      if (!addr || !size) {
         bufferObject = 0;
         stride = 0;
      } else {
         auto buffer = getDataBuffer(addr, size, true, false);
         bufferObject = buffer->object;

         needMemoryBarrier |= buffer->dirtyMap;
         buffer->dirtyMap = false;
      }

      if (mActiveShader->fetch->mAttribBufferCache[i].object != bufferObject
       || mActiveShader->fetch->mAttribBufferCache[i].stride != stride) {
         mActiveShader->fetch->mAttribBufferCache[i].object = bufferObject;
         mActiveShader->fetch->mAttribBufferCache[i].stride = stride;
         gl::glBindVertexBuffer(i, bufferObject, 0, stride);
      }
   }

   if (needMemoryBarrier) {
      gl::glMemoryBarrier(gl::GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
   }

   return true;
}

bool
GLDriver::checkAttribBuffersBound()
{
   // We assume that checkActiveAttribBuffers has already failed the
   //  draw if there is something wrong at a higher level.

   for (auto i = 0; i < mActiveShader->fetch->attribs.size(); ++i) {
      auto &attrib = mActiveShader->fetch->attribs[i];
      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0 + attrib.buffer) * 7;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);

      if (!sq_vtx_constant_word0.BASE_ADDRESS()) {
         return false;
      }
   }

   return true;
}

bool GLDriver::parseFetchShader(FetchShader &shader, void *buffer, size_t size)
{
   auto program = reinterpret_cast<latte::ControlFlowInst *>(buffer);

   for (auto i = 0; i < size / 2; i++) {
      auto &cf = program[i];

      switch (cf.word1.CF_INST()) {
      case latte::SQ_CF_INST_VTX:
      case latte::SQ_CF_INST_VTX_TC:
      {
         auto vfPtr = reinterpret_cast<latte::VertexFetchInst *>(program + cf.word0.ADDR());
         auto count = (cf.word1.COUNT() | (cf.word1.COUNT_3() << 3)) + 1;

         for (auto j = 0u; j < count; ++j) {
            auto &vf = vfPtr[j];

            if (vf.word0.VTX_INST() != latte::SQ_VTX_INST_SEMANTIC) {
               gLog->error("Unexpected fetch shader VTX_INST {}", vf.word0.VTX_INST());
               continue;
            }

            // Parse new attrib
            shader.attribs.emplace_back();
            auto &attrib = shader.attribs.back();
            attrib.bytesPerElement = vf.word0.MEGA_FETCH_COUNT() + 1;
            attrib.format = vf.word1.DATA_FORMAT();
            attrib.buffer = vf.word0.BUFFER_ID();
            attrib.location = vf.gpr.DST_GPR();
            attrib.offset = vf.word2.OFFSET();
            attrib.formatComp = vf.word1.FORMAT_COMP_ALL();
            attrib.numFormat = vf.word1.NUM_FORMAT_ALL();
            attrib.endianSwap = vf.word2.ENDIAN_SWAP();
            attrib.dstSel[0] = vf.word1.DST_SEL_X();
            attrib.dstSel[1] = vf.word1.DST_SEL_Y();
            attrib.dstSel[2] = vf.word1.DST_SEL_Z();
            attrib.dstSel[3] = vf.word1.DST_SEL_W();
            attrib.type = vf.word0.FETCH_TYPE();
            attrib.srcSelX = vf.word0.SRC_SEL_X();
         }
         break;
      }
      case latte::SQ_CF_INST_RETURN:
      case latte::SQ_CF_INST_END_PROGRAM:
         return true;
      default:
         gLog->error("Unexpected fetch shader instruction {}", cf.word1.CF_INST());
      }

      if (cf.word1.END_OF_PROGRAM()) {
         return true;
      }
   }

   return false;
}

static const char *
getGLSLDataInFormat(latte::SQ_DATA_FORMAT format, latte::SQ_NUM_FORMAT num, latte::SQ_FORMAT_COMP comp)
{
   switch (format) {
   case latte::SQ_DATA_FORMAT::FMT_2_10_10_10:
   case latte::SQ_DATA_FORMAT::FMT_10_10_10_2:
      return "uint";
   }

   auto channels = latte::getDataFormatComponents(format);

   switch (channels) {
   case 1:
      return "uint";
   case 2:
      return "uvec2";
   case 3:
      return "uvec3";
   case 4:
      return "uvec4";
   default:
      decaf_abort(fmt::format("Unimplemented attribute channel count: {} for {}", channels, format));
   }
}

bool GLDriver::compileVertexShader(VertexShader &vertex, FetchShader &fetch, uint8_t *buffer, size_t size, bool isScreenSpace)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_vs_out_config = getRegister<latte::SPI_VS_OUT_CONFIG>(latte::Register::SPI_VS_OUT_CONFIG);
   std::array<FetchShader::Attrib *, 32> semanticAttribs;
   semanticAttribs.fill(nullptr);

   glsl2::Shader shader;
   shader.type = glsl2::Shader::VertexShader;

   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::VS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   vertex.disassembly = latte::disassemble(gsl::make_span(buffer, size));

   if (!glsl2::translate(shader, gsl::make_span(buffer, size))) {
      gLog->error("Failed to decode vertex shader\n{}", vertex.disassembly);
      return false;
   }

   vertex.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::memory_buffer out;
   fmt::format_to(out, "{}", shader.fileHeader);

   fmt::format_to(out,
      "#define bswap16(v) (packUnorm4x8(unpackUnorm4x8(v).yxwz))\n"
      "#define bswap32(v) (packUnorm4x8(unpackUnorm4x8(v).wzyx))\n"
      "#define signext2(v) ((v ^ 0x2) - 0x2)\n"
      "#define signext8(v) ((v ^ 0x80) - 0x80)\n"
      "#define signext10(v) ((v ^ 0x200) - 0x200)\n"
      "#define signext16(v) ((v ^ 0x8000) - 0x8000)\n");

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      fmt::format_to(out, "// {}", latte::getDataFormatName(attrib.format));
      if (attrib.formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
         fmt::format_to(out, " SIGNED");
      } else {
         fmt::format_to(out, " UNSIGNED");
      }
      if (attrib.numFormat == latte::SQ_NUM_FORMAT::INT) {
         fmt::format_to(out, " INT");
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT::NORM) {
         fmt::format_to(out, " NORM");
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT::SCALED) {
         fmt::format_to(out, " SCALED");
      }
      if (attrib.endianSwap == latte::SQ_ENDIAN::NONE) {
         fmt::format_to(out, " SWAP_NONE");
      } else if (attrib.endianSwap == latte::SQ_ENDIAN::SWAP_8IN32) {
         fmt::format_to(out, " SWAP_8IN32");
      } else if (attrib.endianSwap == latte::SQ_ENDIAN::SWAP_8IN16) {
         fmt::format_to(out, " SWAP_8IN16");
      } else if (attrib.endianSwap == latte::SQ_ENDIAN::AUTO) {
         fmt::format_to(out, " SWAP_AUTO");
      }

      fmt::format_to(out, "\nlayout(location = {}) in {} fs_out_{};\n",
                     attrib.location,
                     getGLSLDataInFormat(attrib.format, attrib.numFormat, attrib.formatComp),
                     attrib.location);
   }
   fmt::format_to(out, "\n");

   // Vertex Shader Exports
   decaf_check(!spi_vs_out_config.VS_PER_COMPONENT());
   vertex.outputMap.fill(0xff);

   for (auto i = 0u; i <= spi_vs_out_config.VS_EXPORT_COUNT(); i++) {
      auto regId = i / 4;
      auto spi_vs_out_id = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + 4 * regId);

      auto semanticNum = i % 4;
      uint8_t semanticId = 0xff;

      if (semanticNum == 0) {
         semanticId = spi_vs_out_id.SEMANTIC_0();
      } else if (semanticNum == 1) {
         semanticId = spi_vs_out_id.SEMANTIC_1();
      } else if (semanticNum == 2) {
         semanticId = spi_vs_out_id.SEMANTIC_2();
      } else if (semanticNum == 3) {
         semanticId = spi_vs_out_id.SEMANTIC_3();
      }

      if (semanticId == 0xff) {
         // Stop looping when we hit the end marker
         break;
      }

      decaf_check(vertex.outputMap[semanticId] == 0xff);
      vertex.outputMap[semanticId] = i;

      fmt::format_to(out, "layout(location = {}) out vec4 vs_out_{};\n",
                     i, semanticId);
   }
   fmt::format_to(out, "\n");

   // Transform feedback outputs
   for (auto i = 0u; i < latte::MaxStreamOutBuffers; ++i) {
      vertex.usedFeedbackBuffers[i] = !shader.feedbacks[i].empty();

      if (vertex.usedFeedbackBuffers[i]) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
         auto stride = vgt_strmout_vtx_stride * 4;

         if (NVIDIA_GLSL_WORKAROUND) {
            fmt::format_to(out,
               "layout(xfb_buffer = {}) out;\n"
               "layout(xfb_stride = {}) out feedback_block{} {{\n",
               i, stride, i);
         } else {
            fmt::format_to(out,
               "layout(xfb_buffer = {}, xfb_stride = {}) out feedback_block{} {{\n",
               i, stride, i);
         }

         for (auto &xfb : shader.feedbacks[i]) {
            fmt::format_to(out, "   layout(xfb_offset = {}) out ", xfb.offset);

            if (xfb.size == 1) {
               fmt::format_to(out, "float");
            } else {
               fmt::format_to(out, "vec{}", xfb.size);
            }

            fmt::format_to(out, " feedback_{}_{};\n", xfb.streamIndex, xfb.offset);
         }

         fmt::format_to(out, "}};\n");
      }
   }
   fmt::format_to(out, "\n");

   if (isScreenSpace) {
      vertex.isScreenSpace = true;
      fmt::format_to(out, "uniform vec4 uViewport;\n");
   }

   fmt::format_to(out, "void main()\n{{\n{}", shader.codeHeader);

   // Assign fetch shader output to our GPR
   for (auto i = 0u; i < 32; ++i) {
      auto sq_vtx_semantic = getRegister<latte::SQ_VTX_SEMANTIC_N>(latte::Register::SQ_VTX_SEMANTIC_0 + i * 4);
      auto id = sq_vtx_semantic.SEMANTIC_ID();

      if (id == 0xff) {
         continue;
      }

      auto attrib = semanticAttribs[id];

      if (!attrib) {
         gLog->error("Invalid semantic mapping: {}", id);
         continue;
      }


      auto name = fmt::format("fs_out_{}", attrib->location);
      auto channels = latte::getDataFormatComponents(attrib->format);
      auto isFloat = latte::getDataFormatIsFloat(attrib->format);

      std::string chanVal[4];
      uint32_t chanBitCount[4];

      if (attrib->format == latte::SQ_DATA_FORMAT::FMT_10_10_10_2 || attrib->format == latte::SQ_DATA_FORMAT::FMT_2_10_10_10) {
         decaf_check(channels == 4);

         auto val = name;
         if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN32) {
            val = "bswap32(" + val + ")";
         } else if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN16) {
            decaf_abort("Unexpected 8IN16 swap for 10_10_10_2");
         } else if (attrib->endianSwap == latte::SQ_ENDIAN::NONE) {
            // Nothing to do
         } else {
            decaf_abort("Unexpected endian swap mode");
         }

         if (attrib->format == latte::SQ_DATA_FORMAT::FMT_10_10_10_2) {
            chanVal[0] = "((" + val + " >> 22) & 0x3ff)";
            chanVal[1] = "((" + val + " >> 12) & 0x3ff)";
            chanVal[2] = "((" + val + " >> 2) & 0x3ff)";
            chanVal[3] = "((" + val + " >> 0) & 0x3)";
         } else if (attrib->format == latte::SQ_DATA_FORMAT::FMT_2_10_10_10) {
            chanVal[3] = "((" + val + " >> 30) & 0x3)";
            chanVal[2] = "((" + val + " >> 20) & 0x3ff)";
            chanVal[1] = "((" + val + " >> 10) & 0x3ff)";
            chanVal[0] = "((" + val + " >> 0) & 0x3ff)";
         } else {
            decaf_abort("Unexpected format");
         }

         if (attrib->formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
            chanVal[0] = "int(signext10(" + chanVal[0] + "))";
            chanVal[1] = "int(signext10(" + chanVal[1] + "))";
            chanVal[2] = "int(signext10(" + chanVal[2] + "))";
            chanVal[3] = "int(" + chanVal[3] + ")";
         } else {
            // Good to go!
         }

         chanBitCount[0] = 10;
         chanBitCount[1] = 10;
         chanBitCount[2] = 10;
         chanBitCount[3] = 2;
      } else {
         static const char * ChannelSelNorm[] = { "x" ,"y", "z", "w" };
         auto compBits = latte::getDataFormatComponentBits(attrib->format);

         for (auto ch = 0u; ch < channels; ++ch) {
            auto &val = chanVal[ch];
            val = name;

            if (attrib->endianSwap == latte::SQ_ENDIAN::NONE) {
               // Nothing to do except select the appropriate component.

               if (channels > 1) {
                  val = val + "." + ChannelSelNorm[ch];
               }
            } else {
               if (compBits == 32) {
                  if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN32) {
                     if (channels > 1) {
                        val = val + "." + ChannelSelNorm[ch];
                     }

                     val = "bswap32(" + val + ")";
                  } else {
                     decaf_abort("Unexpected endian swap mode for 32-bit components");
                  }
               } else if (compBits == 16) {
                  if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN16) {
                     if (channels > 1) {
                        val = val + "." + ChannelSelNorm[ch];
                     }

                     val = "bswap16(" + val + ")";
                  } else {
                     decaf_abort("Unexpected endian swap mode for 16-bit components");
                  }
               } else if (compBits == 8) {
                  static const char * ChannelSel8In16[] = { "y", "x", "w", "z" };
                  static const char * ChannelSel8In32[] = { "w", "z", "y", "x" };

                  if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN16) {
                     decaf_check(channels == 2 || channels == 4);
                     val = val + "." + ChannelSel8In16[ch];
                  } else if (attrib->endianSwap == latte::SQ_ENDIAN::SWAP_8IN32) {
                     decaf_check(channels == 4);
                     val = val + "." + ChannelSel8In32[ch];
                  } else {
                     decaf_abort("Unexpected endian swap mode for 8-bit components");
                  }
               } else {
                  decaf_abort("Unexpected component bit count with swapping");
               }
            }

            if (isFloat) {
               if (compBits == 32) {
                  val = "uintBitsToFloat(" + val + ")";
               } else if (compBits == 16) {
                  val = "unpackHalf2x16(" + val + ").x";
               } else {
                  decaf_abort("Unexpected float component bit count");
               }
            } else {
               if (attrib->formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
                  if (compBits == 8) {
                     val = "int(signext8(" + val + "))";
                  } else if (compBits == 16) {
                     val = "int(signext16(" + val + "))";
                  } else if (compBits == 32) {
                     val = "int(" + val + ")";
                  } else {
                     decaf_abort("Unexpected signed component bit count");
                  }
               } else {
                  // Already the right format!
               }
            }

            chanBitCount[ch] = compBits;
         }
      }

      for (auto ch = 0u; ch < channels; ++ch) {
         if (attrib->numFormat == latte::SQ_NUM_FORMAT::NORM) {
            uint32_t valMax = (1ul << chanBitCount[ch]) - 1;

            if (attrib->formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
               chanVal[ch] = fmt::format("clamp(float({}) / {}.0, -1.0, 1.0)", chanVal[ch], valMax / 2);
            } else {
               chanVal[ch] = fmt::format("float({}) / {}.0", chanVal[ch], valMax);
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT::INT) {
            if (attrib->formatComp == latte::SQ_FORMAT_COMP::SIGNED) {
               chanVal[ch] = "intBitsToFloat(int(" + chanVal[ch] + "))";
            } else {
               chanVal[ch] = "uintBitsToFloat(uint(" + chanVal[ch] + "))";
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT::SCALED) {
            chanVal[ch] = "float(" + chanVal[ch] + ")";
         } else {
            decaf_abort("Unexpected attribute number format");
         }
      }

      if (channels == 1) {
         fmt::format_to(out, "float _{} = {};\n", name, chanVal[0]);
      } else if (channels == 2) {
         fmt::format_to(out,
                        "vec2 _{} = vec2(\n"
                        "   {},\n"
                        "   {});\n",
                        name, chanVal[0], chanVal[1]);
      } else if (channels == 3) {
         fmt::format_to(out,
                        "vec3 _{} = vec3(\n"
                        "   {},\n"
                        "   {},\n"
                        "   {});\n",
                        name, chanVal[0], chanVal[1], chanVal[2]);
      } else if (channels == 4) {
         fmt::format_to(out,
                        "vec4 _{} = vec4(\n"
                        "   {},\n"
                        "   {},\n"
                        "   {},\n"
                        "   {});\n",
                        name, chanVal[0], chanVal[1], chanVal[2], chanVal[3]);
      } else {
         decaf_abort("Unexpected format channel count");
      }
      name = "_" + name;

      // Write the register assignment
      fmt::format_to(out, "R[{}] = ", i + 1);

      switch (channels) {
      case 1:
         fmt::format_to(out, "vec4({}, 0.0, 0.0, 1.0);\n", name);
         break;
      case 2:
         fmt::format_to(out, "vec4({}, 0.0, 1.0);\n", name);
         break;
      case 3:
         fmt::format_to(out, "vec4({}, 1.0);\n", name);
         break;
      case 4:
         fmt::format_to(out, "{};\n", name);
         break;
      }
   }

   fmt::format_to(out, "\n{}\n", shader.codeBody);

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_TYPE::POS:
         if (!isScreenSpace) {
            fmt::format_to(out, "gl_Position = exp_position_{};\n", exp.id);
         } else {
            fmt::format_to(out, "gl_Position = (exp_position_{} - vec4(uViewport.xy, 0.0, 0.0)) * vec4(uViewport.zw, 1.0, 1.0);\n", exp.id);
         }
         break;
      case latte::SQ_EXPORT_TYPE::PARAM: {
         decaf_check(!spi_vs_out_config.VS_PER_COMPONENT());

         auto regId = exp.id / 4;
         auto spi_vs_out_id = getRegister<latte::SPI_VS_OUT_ID_N>(latte::Register::SPI_VS_OUT_ID_0 + 4 * regId);

         auto semanticNum = exp.id % 4;
         uint8_t semanticId = 0xff;

         if (semanticNum == 0) {
            semanticId = spi_vs_out_id.SEMANTIC_0();
         } else if (semanticNum == 1) {
            semanticId = spi_vs_out_id.SEMANTIC_1();
         } else if (semanticNum == 2) {
            semanticId = spi_vs_out_id.SEMANTIC_2();
         } else if (semanticNum == 3) {
            semanticId = spi_vs_out_id.SEMANTIC_3();
         }

         if (semanticId != 0xff) {
            fmt::format_to(out, "vs_out_{} = exp_param_{};\n", semanticId, exp.id);
         } else {
            // This just helps when debugging to understand why it is missing...
            fmt::format_to(out, "// vs_out_none = exp_param_{};\n", exp.id);
         }
      } break;
      case latte::SQ_EXPORT_TYPE::PIXEL:
         decaf_abort("Unexpected pixel export in vertex shader.");
      }
   }

   fmt::format_to(out, "}}\n");
   fmt::format_to(out, "/* VERTEX SHADER DISASSEMBLY\n{}\n*/\n", vertex.disassembly);
   fmt::format_to(out, "/* FETCH SHADER DISASSEMBLY\n{}\n*/\n", fetch.disassembly);
   vertex.code = to_string(out);
   return true;
}

bool GLDriver::compilePixelShader(PixelShader &pixel, VertexShader &vertex, uint8_t *buffer, size_t size)
{
   auto sq_config = getRegister<latte::SQ_CONFIG>(latte::Register::SQ_CONFIG);
   auto spi_ps_in_control_0 = getRegister<latte::SPI_PS_IN_CONTROL_0>(latte::Register::SPI_PS_IN_CONTROL_0);
   auto spi_ps_in_control_1 = getRegister<latte::SPI_PS_IN_CONTROL_1>(latte::Register::SPI_PS_IN_CONTROL_1);
   auto cb_shader_mask = getRegister<latte::CB_SHADER_MASK>(latte::Register::CB_SHADER_MASK);
   auto db_shader_control = getRegister<latte::DB_SHADER_CONTROL>(latte::Register::DB_SHADER_CONTROL);
   auto sx_alpha_test_control = getRegister<latte::SX_ALPHA_TEST_CONTROL>(latte::Register::SX_ALPHA_TEST_CONTROL);

   decaf_assert(!db_shader_control.STENCIL_REF_EXPORT_ENABLE(), "Stencil exports not implemented");

   glsl2::Shader shader;
   shader.type = glsl2::Shader::PixelShader;

   // Gather Samplers
   for (auto i = 0; i < latte::MaxSamplers; ++i) {
      auto resourceOffset = (latte::SQ_RES_OFFSET::PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   pixel.disassembly = latte::disassemble(gsl::make_span(buffer, size));

   if (!glsl2::translate(shader, gsl::make_span(buffer, size))) {
      gLog->error("Failed to decode pixel shader\n{}", pixel.disassembly);
      return false;
   }

   pixel.samplerUsage = shader.samplerUsage;
   pixel.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::memory_buffer out;
   fmt::format_to(out, "{}", shader.fileHeader);
   fmt::format_to(out, "uniform float uAlphaRef;\n");

   auto z_order = db_shader_control.Z_ORDER();
   auto early_z = (z_order == latte::DB_Z_ORDER::EARLY_Z_THEN_LATE_Z || z_order == latte::DB_Z_ORDER::EARLY_Z_THEN_RE_Z);
   if (early_z) {
      if (sx_alpha_test_control.ALPHA_TEST_ENABLE() && !sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
         gLog->debug("Ignoring early-Z because alpha test is enabled");
         early_z = false;
      } else if (db_shader_control.KILL_ENABLE()) {
         gLog->debug("Ignoring early-Z because shader discard is enabled");
         early_z = false;
      } else {
         decaf_assert(!shader.usesDiscard, "Shader uses discard but KILL_ENABLE is not set");
         for (auto &exp : shader.exports) {
            if (exp.type == latte::SQ_EXPORT_TYPE::PIXEL && exp.id == 61) {
               gLog->debug("Ignoring early-Z because shader writes gl_FragDepth");
               early_z = false;
               break;
            }
         }
      }
      if (early_z) {
         fmt::format_to(out, "layout(early_fragment_tests) in;\n");
      }
   }

   if (spi_ps_in_control_0.POSITION_ENA()) {
      if (!spi_ps_in_control_0.POSITION_CENTROID()) {
         fmt::format_to(out, "layout(pixel_center_integer) ");
      }
      fmt::format_to(out, "in vec4 gl_FragCoord;\n");
   }

   // Pixel Shader Inputs
   std::array<bool, 256> semanticUsed = { false };
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_N>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      auto semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      if (semanticId == 0xff) {
         // Missing semantic means we need to apply the default values instead...
         continue;
      }

      if (semanticUsed[semanticId]) {
         continue;
      } else {
         semanticUsed[semanticId] = true;
      }

      fmt::format_to(out, "layout(location = {})", vsOutputLoc);

      if (spi_ps_input_cntl.FLAT_SHADE()) {
         fmt::format_to(out, " flat");
      }

      fmt::format_to(out, " in vec4 vs_out_{};\n", semanticId);
   }
   fmt::format_to(out, "\n");

   // Pixel Shader Exports
   auto maskBits = cb_shader_mask.value;

   for (auto i = 0; i < 8; ++i) {
      if (maskBits & 0xf) {
         fmt::format_to(out, "out vec4 ps_out_{};\n", i);
      }

      maskBits >>= 4;
   }
   fmt::format_to(out, "\n");

   fmt::format_to(out, "void main()\n{{\n{}", shader.codeHeader);

   // Assign vertex shader output to our GPR
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_N>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      uint8_t semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      fmt::format_to(out, "R[{}] = ", i);

      if (vsOutputLoc != 0xff) {
          fmt::format_to(out, "vs_out_{}", semanticId);
      } else {
         if (spi_ps_input_cntl.DEFAULT_VAL() == 0) {
            fmt::format_to(out, "vec4(0, 0, 0, 0)");
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 1) {
            fmt::format_to(out, "vec4(0, 0, 0, 1)");
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 2) {
            fmt::format_to(out, "vec4(1, 1, 1, 0)");
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 3) {
            fmt::format_to(out, "vec4(1, 1, 1, 1)");
         } else {
            decaf_abort("Invalid PS input DEFAULT_VAL");
         }
      }

      fmt::format_to(out, ";\n");
   }

   if (spi_ps_in_control_0.POSITION_ENA()) {
      fmt::format_to(out, "R[{}] = gl_FragCoord;", spi_ps_in_control_0.POSITION_ADDR());
   }

   decaf_assert(!spi_ps_in_control_0.PARAM_GEN(),
                fmt::format("Unsupported spi_ps_in_control_0.PARAM_GEN {}, PARAM_GEN_ADDR {}",
                            spi_ps_in_control_0.PARAM_GEN(),
                            spi_ps_in_control_0.PARAM_GEN_ADDR()));
   decaf_check(!spi_ps_in_control_1.GEN_INDEX_PIX());
   decaf_check(!spi_ps_in_control_1.FIXED_PT_POSITION_ENA());

   fmt::format_to(out, "\n{}\n", shader.codeBody);

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_TYPE::PIXEL:
         if (exp.id == 61) {
            if (!db_shader_control.Z_EXPORT_ENABLE()) {
               gLog->warn("Depth export is masked by db_shader_control");
            } else {
               fmt::format_to(out, "gl_FragDepth = exp_pixel_{}.x;\n", exp.id);
            }
         } else {
            auto mask = (cb_shader_mask.value >> (4 * exp.id)) & 0x0F;

            if (!mask) {
               gLog->warn("Export is masked by cb_shader_mask");
            } else {
               std::string strMask;

               if (mask & (1 << 0)) {
                  strMask.push_back('x');
               }

               if (mask & (1 << 1)) {
                  strMask.push_back('y');
               }

               if (mask & (1 << 2)) {
                  strMask.push_back('z');
               }

               if (mask & (1 << 3)) {
                  strMask.push_back('w');
               }

               if (sx_alpha_test_control.ALPHA_TEST_ENABLE() && !sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
                  fmt::format_to(out, "// Alpha Test ");

                  switch (sx_alpha_test_control.ALPHA_FUNC()) {
                  case latte::REF_FUNC::NEVER:
                     fmt::format_to(out,
                                    "REF_NEVER\n"
                                    "discard;\n");
                     break;
                  case latte::REF_FUNC::LESS:
                     fmt::format_to(out,
                                    "REF_LESS\n"
                                    "if (!(exp_pixel_{}.w < uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::EQUAL:
                     fmt::format_to(out,
                                    "REF_EQUAL\n"
                                    "if (!(exp_pixel_{}.w == uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::LESS_EQUAL:
                     fmt::format_to(out,
                                    "REF_LESS_EQUAL\n"
                                    "if (!(exp_pixel_{}.w <= uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::GREATER:
                     fmt::format_to(out,
                                    "REF_GREATER\n"
                                    "if (!(exp_pixel_{}.w > uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::NOT_EQUAL:
                     fmt::format_to(out,
                                    "REF_NOT_EQUAL\n"
                                    "if (!(exp_pixel_{}.w != uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::GREATER_EQUAL:
                     fmt::format_to(out,
                                    "REF_GREATER_EQUAL\n"
                                    "if (!(exp_pixel_{}.w >= uAlphaRef)) {{\n"
                                    "   discard;\n}}\n", exp.id);
                     break;
                  case latte::REF_FUNC::ALWAYS:
                     fmt::format_to(out, "REF_ALWAYS\n");
                     break;
                  }
               }

               fmt::format_to(out, "ps_out_{}.{} = exp_pixel_{}.{};\n",
                              exp.id, strMask, exp.id, strMask);
            }
         }
         break;
      case latte::SQ_EXPORT_TYPE::POS:
         decaf_abort("Unexpected position export in pixel shader.");
         break;
      case latte::SQ_EXPORT_TYPE::PARAM:
         decaf_abort("Unexpected parameter export in pixel shader.");
         break;
      }
   }

   fmt::format_to(out, "}}\n/* PIXEL SHADER DISASSEMBLY\n{}\n*/\n", pixel.disassembly);
   pixel.code = to_string(out);
   return true;
}

} // namespace opengl

#endif // ifdef DECAF_GL
