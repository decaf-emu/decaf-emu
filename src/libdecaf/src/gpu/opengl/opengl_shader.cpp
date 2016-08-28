#include "common/decaf_assert.h"
#include "common/log.h"
#include "common/murmur3.h"
#include "common/platform_dir.h"
#include "common/strutils.h"
#include "decaf_config.h"
#include "glsl2_translate.h"
#include "gpu/gpu_utilities.h"
#include "gpu/latte_registers.h"
#include "gpu/microcode/latte_disassembler.h"
#include "opengl_constants.h"
#include "opengl_driver.h"
#include <fstream>
#include <glbinding/gl/gl.h>
#include <spdlog/spdlog.h>

namespace gpu
{

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
dumpRawShader(const std::string &type, ppcaddr_t data, uint32_t size, bool isSubroutine = false)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   std::string filePath = fmt::format("dump/gpu_{}_{:08x}.txt", type, data);

   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   auto file = std::ofstream{ filePath, std::ofstream::out };

   // Disassemble
   auto output = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(data), size), isSubroutine);

   file << output << std::endl;
}

static void
dumpTranslatedShader(const std::string &type, ppcaddr_t data, const std::string &shaderSource)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   std::string filePath = fmt::format("dump/gpu_{}_{:08x}.glsl", type, data);

   if (platform::fileExists(filePath)) {
      return;
   }

   platform::createDirectory("dump");

   auto file = std::ofstream{ filePath, std::ofstream::out };

   file << shaderSource << std::endl;
}

static gl::GLenum
getDataFormatGlType(latte::SQ_DATA_FORMAT format)
{
   // Some Special Cases
   switch (format) {
   case latte::FMT_10_10_10_2:
   case latte::FMT_2_10_10_10:
      return gl::GL_UNSIGNED_INT;
   }

   uint32_t bitCount = getDataFormatComponentBits(format);
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

// Must be called with mResourceMutex locked; returns whether the shader
//  was invalidated
template <typename ShaderPtrType> static bool
invalidateShaderIfChanged_locked(ShaderPtrType &shader,
                                 uint64_t shaderKey,
                                 std::unordered_map<uint64_t, ShaderPtrType> &shaders)
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
   MurmurHash3_x64_128(mem::translate(shader->cpuMemStart), shader->cpuMemEnd - shader->cpuMemStart, 0, newHash);
   if (newHash[0] == shader->cpuMemHash[0] && newHash[1] == shader->cpuMemHash[1]) {
      shader->needRebuild = false;
      return false;
   }

   // The shader contents have changed, so delete the existing object
   deleteShaderObject(shader);
   shader->object = 0;

   shader->refCount--;
   if (shader->refCount == 0) {
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
   bool isScreenSpace = (vgt_primitive_type.PRIM_TYPE() == latte::VGT_DI_PT_RECTLIST);

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

   auto fsPgmAddress = pgm_start_fs.PGM_START() << 8;
   auto vsPgmAddress = pgm_start_vs.PGM_START() << 8;
   auto psPgmAddress = pgm_start_ps.PGM_START() << 8;
   auto fsPgmSize = pgm_size_fs.PGM_SIZE() << 3;
   auto vsPgmSize = pgm_size_vs.PGM_SIZE() << 3;
   auto psPgmSize = pgm_size_ps.PGM_SIZE() << 3;
   auto z_order = db_shader_control.Z_ORDER();
   auto early_z = (z_order == latte::DB_EARLY_Z_THEN_LATE_Z || z_order == latte::DB_EARLY_Z_THEN_RE_Z);
   auto alphaTestFunc = sx_alpha_test_control.ALPHA_FUNC();

   if (!sx_alpha_test_control.ALPHA_TEST_ENABLE() || sx_alpha_test_control.ALPHA_TEST_BYPASS()) {
      alphaTestFunc = latte::REF_ALWAYS;
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
   vsShaderKey ^= (isScreenSpace ? 1 : 0) << 31;

   if (vgt_strmout_en.STREAMOUT()) {
      vsShaderKey |= 1;

      for (auto i = 0u; i < latte::MaxStreamOutBuffers; ++i) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * i);
         vsShaderKey ^= vgt_strmout_vtx_stride << (1 + 7 * i);
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
      std::unique_lock<std::mutex> mResourceMutex;

      if (invalidateShaderIfChanged_locked(pipeline.fetch, fsShaderKey, mFetchShaders)
       || invalidateShaderIfChanged_locked(pipeline.vertex, vsShaderKey, mVertexShaders)
       || invalidateShaderIfChanged_locked(pipeline.pixel, psShaderKey, mPixelShaders)) {
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
      std::unique_lock<std::mutex> mResourceMutex;

      // Parse fetch shader if needed
      auto &fetchShader = mFetchShaders[fsShaderKey];
      invalidateShaderIfChanged_locked(fetchShader, fsShaderKey, mFetchShaders);

      if (!fetchShader) {
         auto aluDivisor0 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_0);
         auto aluDivisor1 = getRegister<uint32_t>(latte::Register::VGT_INSTANCE_STEP_RATE_1);

         fetchShader = new FetchShader {};
         fetchShader->cpuMemStart = fsPgmAddress;
         fetchShader->cpuMemEnd = fsPgmAddress + fsPgmSize;
         MurmurHash3_x64_128(mem::translate(fetchShader->cpuMemStart),
                             fetchShader->cpuMemEnd - fetchShader->cpuMemStart,
                             0, fetchShader->cpuMemHash);
         fetchShader->dirtyMemory = false;

         dumpRawShader("fetch", fsPgmAddress, fsPgmSize, true);
         fetchShader->disassembly = latte::disassemble(gsl::as_span(mem::translate<uint8_t>(fsPgmAddress), fsPgmSize), true);

         if (!parseFetchShader(*fetchShader, make_virtual_ptr<void>(fsPgmAddress), fsPgmSize)) {
            gLog->error("Failed to parse fetch shader");
            return false;
         }

         // Setup attrib format
         gl::glCreateVertexArrays(1, &fetchShader->object);
         if (decaf::config::gpu::debug) {
            std::string label = fmt::format("fetch shader @ 0x{:08X}", fsPgmAddress);
            gl::glObjectLabel(gl::GL_VERTEX_ARRAY, fetchShader->object, -1, label.c_str());
         }

         auto bufferUsed = std::array<bool, latte::MaxAttributes> { false };
         auto bufferDivisor = std::array<uint32_t, latte::MaxAttributes> { 0 };

         for (auto &attrib : fetchShader->attribs) {
            auto resourceId = attrib.buffer + latte::SQ_VS_RESOURCE_BASE;

            if (resourceId >= latte::SQ_VS_ATTRIB_RESOURCE_0 && resourceId < latte::SQ_VS_ATTRIB_RESOURCE_0 + 0x10) {
               auto attribBufferId = resourceId - latte::SQ_VS_ATTRIB_RESOURCE_0;
               auto type = getDataFormatGlType(attrib.format);
               auto components = getDataFormatComponents(attrib.format);
               auto divisor = 0u;

               gl::glEnableVertexArrayAttrib(fetchShader->object, attrib.location);
               gl::glVertexArrayAttribIFormat(fetchShader->object, attrib.location, components, type, attrib.offset);
               gl::glVertexArrayAttribBinding(fetchShader->object, attrib.location, attribBufferId);

               if (attrib.type == latte::SQ_VTX_FETCH_TYPE::SQ_VTX_FETCH_INSTANCE_DATA) {
                  if (attrib.srcSelX == latte::SQ_SEL_W) {
                     divisor = 1;
                  } else if (attrib.srcSelX == latte::SQ_SEL_Y) {
                     divisor = aluDivisor0;
                  } else if (attrib.srcSelX == latte::SQ_SEL_Z) {
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

         for (auto bufferId = 0; bufferId < latte::MaxAttributes; ++bufferId) {
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
      invalidateShaderIfChanged_locked(vertexShader, vsShaderKey, mVertexShaders);

      if (!vertexShader) {
         vertexShader = new VertexShader;

         vertexShader->cpuMemStart = vsPgmAddress;
         vertexShader->cpuMemEnd = vsPgmAddress + vsPgmSize;
         MurmurHash3_x64_128(mem::translate(vertexShader->cpuMemStart),
                             vertexShader->cpuMemEnd - vertexShader->cpuMemStart,
                             0, vertexShader->cpuMemHash);
         vertexShader->dirtyMemory = false;

         dumpRawShader("vertex", vsPgmAddress, vsPgmSize);

         if (!compileVertexShader(*vertexShader, *fetchShader, make_virtual_ptr<uint8_t>(vsPgmAddress), vsPgmSize, isScreenSpace)) {
            gLog->error("Failed to recompile vertex shader");
            return false;
         }

         dumpTranslatedShader("vertex", vsPgmAddress, vertexShader->code);

         // Create OpenGL Shader
         const gl::GLchar *code[] = { vertexShader->code.c_str() };
         vertexShader->object = gl::glCreateShaderProgramv(gl::GL_VERTEX_SHADER, 1, code);
         if (decaf::config::gpu::debug) {
            std::string label = fmt::format("vertex shader @ 0x{:08X}", vsPgmAddress);
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
         invalidateShaderIfChanged_locked(pixelShader, psShaderKey, mPixelShaders);

         if (!pixelShader) {
            pixelShader = new PixelShader;

            pixelShader->cpuMemStart = psPgmAddress;
            pixelShader->cpuMemEnd = psPgmAddress + psPgmSize;
            MurmurHash3_x64_128(mem::translate(pixelShader->cpuMemStart),
                                pixelShader->cpuMemEnd - pixelShader->cpuMemStart,
                                0, pixelShader->cpuMemHash);
            pixelShader->dirtyMemory = false;

            dumpRawShader("pixel", psPgmAddress, psPgmSize);

            if (!compilePixelShader(*pixelShader, *vertexShader, make_virtual_ptr<uint8_t>(psPgmAddress), psPgmSize)) {
               gLog->error("Failed to recompile pixel shader");
               return false;
            }

            dumpTranslatedShader("pixel", psPgmAddress, pixelShader->code);

            // Create OpenGL Shader
            const gl::GLchar *code[] = { pixelShader->code.c_str() };
            pixelShader->object = gl::glCreateShaderProgramv(gl::GL_FRAGMENT_SHADER, 1, code);

            if (decaf::config::gpu::debug) {
               std::string label = fmt::format("pixel shader @ 0x{:08X}", psPgmAddress);
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
      if (decaf::config::gpu::debug) {
         std::string label;

         if (pipeline.pixel) {
            label = fmt::format("shader set: fs = 0x{:08X}, vs = 0x{:08X}, ps = 0x{:08X}", fsPgmAddress, vsPgmAddress, psPgmAddress);
         } else {
            label = fmt::format("shader set: fs = 0x{:08X}, vs = 0x{:08X}, ps = none", fsPgmAddress, vsPgmAddress);
         }

         gl::glObjectLabel(gl::GL_PROGRAM_PIPELINE, pipeline.object, -1, label.c_str());
      }

      gl::glUseProgramStages(pipeline.object, gl::GL_VERTEX_SHADER_BIT, pipeline.vertex->object);
      gl::glUseProgramStages(pipeline.object, gl::GL_FRAGMENT_SHADER_BIT, pipeline.pixel ? pipeline.pixel->object : 0);
   }

   // Set active shader
   mActiveShader = &pipeline;

   // Set alpha reference
   if (mActiveShader->pixel && alphaTestFunc != latte::REF_ALWAYS && alphaTestFunc != latte::REF_NEVER) {
      gl::glProgramUniform1f(mActiveShader->pixel->object, mActiveShader->pixel->uniformAlphaRef, sx_alpha_ref.ALPHA_REF);
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
               auto addr = sq_alu_const_cache_vs << 8;
               auto size = sq_alu_const_buffer_size_vs << 8;

               // Check that we can fit the uniform block into OpenGL buffers
               decaf_assert(size <= gpu::opengl::MaxUniformBlockSize,
                  fmt::format("Active uniform block with data size {} greater than what OpenGL supports {}", size, MaxUniformBlockSize));

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
               auto addr = sq_alu_const_cache_ps << 8;
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
GLDriver::getDataBuffer(uint32_t address,
                        uint32_t size,
                        bool isInput,
                        bool isOutput)
{
   decaf_check(size);
   decaf_check(isInput || isOutput);

   DataBuffer *buffer;
   {
      std::unique_lock<std::mutex> lock(mResourceMutex);
      buffer = &mDataBuffers[address];
   }

   if (buffer->object
    && buffer->allocatedSize >= size
    && !(isInput && !buffer->isInput)
    && !(isOutput && !buffer->isOutput)) {
      return buffer;  // No changes needed
   }

   auto oldObject = buffer->object;
   auto oldSize = buffer->allocatedSize;
   auto oldMappedBuffer = buffer->mappedBuffer;

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

   if (decaf::config::gpu::debug) {
      const char *type = "output-only";

      if (buffer->isInput) {
         type = buffer->isOutput ? "input/output" : "input-only";
      }

      auto label = fmt::format("{} buffer @ 0x{:08X}", type, address);
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

   return buffer;
}

void
GLDriver::downloadDataBuffer(DataBuffer *buffer,
                             uint32_t offset,
                             uint32_t size)
{
   if (buffer->mappedBuffer) {
      // We only map input-only buffers (see getDataBuffer()), so there's
      //  no need for a memory barrier here.
      decaf_check(!buffer->isOutput);

      memcpy(mem::translate<char>(buffer->cpuMemStart) + offset,
             static_cast<char *>(buffer->mappedBuffer) + offset,
             size);
   } else {
      gl::glGetNamedBufferSubData(buffer->object, offset, size,
                                  mem::translate<char>(buffer->cpuMemStart) + offset);
   }
}

void
GLDriver::uploadDataBuffer(DataBuffer *buffer,
                           uint32_t offset,
                           uint32_t size)
{
   // Avoid uploading the data if it hasn't changed.
   uint64_t newHash[2] = { 0, 0 };
   MurmurHash3_x64_128(mem::translate(buffer->cpuMemStart), buffer->allocatedSize, 0, newHash);

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
                mem::translate<char>(buffer->cpuMemStart) + offset,
                size);
         gl::glFlushMappedNamedBufferRange(buffer->object, offset, size);
         buffer->dirtyMap = true;
      } else {
         gl::glNamedBufferSubData(buffer->object, offset, size,
                                  mem::translate<char>(buffer->cpuMemStart) + offset);
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

   for (auto i = 0u; i < latte::MaxAttributes; ++i) {
      auto resourceOffset = (latte::SQ_VS_ATTRIB_RESOURCE_0 + i) * 7;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word1 = getRegister<latte::SQ_VTX_CONSTANT_WORD1_N>(latte::Register::SQ_VTX_CONSTANT_WORD1_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word2 = getRegister<latte::SQ_VTX_CONSTANT_WORD2_N>(latte::Register::SQ_VTX_CONSTANT_WORD2_0 + 4 * resourceOffset);
      auto sq_vtx_constant_word6 = getRegister<latte::SQ_VTX_CONSTANT_WORD6_N>(latte::Register::SQ_VTX_CONSTANT_WORD6_0 + 4 * resourceOffset);

      auto addr = sq_vtx_constant_word0.BASE_ADDRESS();
      auto size = sq_vtx_constant_word1.SIZE() + 1;
      auto stride = sq_vtx_constant_word2.STRIDE();
      auto bufferObject = gl::GLuint { 0 };

      if (addr == 0 || size == 0) {
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
      auto resourceOffset = (latte::SQ_VS_RESOURCE_BASE + attrib.buffer) * 7;
      auto sq_vtx_constant_word0 = getRegister<latte::SQ_VTX_CONSTANT_WORD0_N>(latte::Register::SQ_VTX_CONSTANT_WORD0_0 + 4 * resourceOffset);

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
         auto vfPtr = reinterpret_cast<latte::VertexFetchInst *>(program + cf.word0.ADDR);
         auto count = ((cf.word1.COUNT_3() << 3) | cf.word1.COUNT()) + 1;

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

   auto channels = getDataFormatComponents(format);

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
      auto resourceOffset = (latte::SQ_VS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   vertex.disassembly = latte::disassemble(gsl::as_span(buffer, size));

   if (!glsl2::translate(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode vertex shader\n{}", vertex.disassembly);
      return false;
   }

   vertex.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::MemoryWriter out;
   out << shader.fileHeader;

   out << "#define bswap16(v) (packUnorm4x8(unpackUnorm4x8(v).yxwz))\n";
   out << "#define bswap32(v) (packUnorm4x8(unpackUnorm4x8(v).wzyx))\n";
   out << "#define signext2(v) ((v ^ 0x2) - 0x2)\n";
   out << "#define signext8(v) ((v ^ 0x80) - 0x80)\n";
   out << "#define signext10(v) ((v ^ 0x200) - 0x200)\n";
   out << "#define signext16(v) ((v ^ 0x8000) - 0x8000)\n";

   // Vertex Shader Inputs
   for (auto &attrib : fetch.attribs) {
      semanticAttribs[attrib.location] = &attrib;

      out << "//";
      out << " " << getDataFormatName(attrib.format);
      if (attrib.formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
         out << " SIGNED";
      } else {
         out << " UNSIGNED";
      }
      if (attrib.numFormat == latte::SQ_NUM_FORMAT_INT) {
         out << " INT";
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT_NORM) {
         out << " NORM";
      } else if (attrib.numFormat == latte::SQ_NUM_FORMAT_SCALED) {
         out << " SCALED";
      }
      if (attrib.endianSwap == latte::SQ_ENDIAN_NONE) {
         out << " SWAP_NONE";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_8IN32) {
         out << " SWAP_8IN32";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_8IN16) {
         out << " SWAP_8IN16";
      } else if (attrib.endianSwap == latte::SQ_ENDIAN_AUTO) {
         out << " SWAP_AUTO";
      }
      out << "\n";

      out << "layout(location = " << attrib.location << ")";
      out << " in "
          << getGLSLDataInFormat(attrib.format, attrib.numFormat, attrib.formatComp)
         << " fs_out_" << attrib.location << ";\n";
   }
   out << '\n';

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

      out << "layout(location = " << i << ")";
      out << " out vec4 vs_out_" << semanticId << ";\n";
   }
   out << '\n';

   // Transform feedback outputs
   for (auto buffer = 0u; buffer < latte::MaxStreamOutBuffers; ++buffer) {
      vertex.usedFeedbackBuffers[buffer] = !shader.feedbacks[buffer].empty();

      if (vertex.usedFeedbackBuffers[buffer]) {
         auto vgt_strmout_vtx_stride = getRegister<uint32_t>(latte::Register::VGT_STRMOUT_VTX_STRIDE_0 + 16 * buffer);
         auto stride = vgt_strmout_vtx_stride * 4;

         if (NVIDIA_GLSL_WORKAROUND) {
            out
               << "layout(xfb_buffer = " << buffer << ") out;\n"
               << "layout(xfb_stride = " << stride
               << ") out feedback_block" << buffer << " {\n";
         } else {
            out
               << "layout(xfb_buffer = " << buffer
               << ", xfb_stride = " << stride
               << ") out feedback_block" << buffer << " {\n";
         }

         for (auto &xfb : shader.feedbacks[buffer]) {
            out << "   layout(xfb_offset = " << xfb.offset << ") out ";

            if (xfb.size == 1) {
               out << "float";
            } else {
               out << "vec" << xfb.size;
            }

            out << " feedback_" << xfb.streamIndex << "_" << xfb.offset << ";\n";
         }

         out << "};\n";
      }
   }
   out << '\n';

   if (isScreenSpace) {
      vertex.isScreenSpace = true;
      out << "uniform vec4 uViewport;\n";
   }

   out
      << "void main()\n"
      << "{\n"
      << shader.codeHeader;

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


      fmt::MemoryWriter nameWriter;
      nameWriter << "fs_out_" << attrib->location;
      auto name = nameWriter.str();
      auto channels = getDataFormatComponents(attrib->format);
      auto isFloat = getDataFormatIsFloat(attrib->format);

      std::string chanVal[4];
      uint32_t chanBitCount[4];

      if (attrib->format == latte::FMT_10_10_10_2 || attrib->format == latte::FMT_2_10_10_10) {
         decaf_check(channels == 4);

         auto val = name;

         if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
            val = "bswap32(" + val + ")";
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
            decaf_abort("Unexpected 8IN16 swap for 10_10_10_2");
         } else if (attrib->endianSwap == latte::SQ_ENDIAN_NONE) {
            // Nothing to do
         } else {
            decaf_abort("Unexpected endian swap mode");
         }

         if (attrib->format == latte::FMT_10_10_10_2) {
            chanVal[0] = std::string("((") + val + std::string(" >> 22) & 0x3ff)");
            chanVal[1] = std::string("((") + val + std::string(" >> 12) & 0x3ff)");
            chanVal[2] = std::string("((") + val + std::string(" >> 2) & 0x3ff)");
            chanVal[3] = std::string("((") + val + std::string(" >> 0) & 0x3)");
         } else if (attrib->format == latte::FMT_2_10_10_10) {
            chanVal[3] = std::string("((") + val + std::string(" >> 30) & 0x3)");
            chanVal[2] = std::string("((") + val + std::string(" >> 20) & 0x3ff)");
            chanVal[1] = std::string("((") + val + std::string(" >> 10) & 0x3ff)");
            chanVal[0] = std::string("((") + val + std::string(" >> 0) & 0x3ff)");
         } else {
            decaf_abort("Unexpected format");
         }

         if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
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

         auto compBits = getDataFormatComponentBits(attrib->format);

         for (auto i = 0u; i < channels; ++i) {
            auto &val = chanVal[i];
            val = name;

            if (attrib->endianSwap == latte::SQ_ENDIAN_NONE) {
               // Nothing to do except select the appropriate component.

               if (channels > 1) {
                  val = val + "." + ChannelSelNorm[i];
               }
            } else {
               if (compBits == 32) {
                  if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
                     if (channels > 1) {
                        val = val + "." + ChannelSelNorm[i];
                     }

                     val = "bswap32(" + val + ")";
                  } else {
                     decaf_abort("Unexpected endian swap mode for 32-bit components");
                  }
               } else if (compBits == 16) {
                  if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
                     if (channels > 1) {
                        val = val + "." + ChannelSelNorm[i];
                     }

                     val = "bswap16(" + val + ")";
                  } else {
                     decaf_abort("Unexpected endian swap mode for 16-bit components");
                  }
               } else if (compBits == 8) {
                  static const char * ChannelSel8In16[] = { "y", "x", "w", "z" };
                  static const char * ChannelSel8In32[] = { "w", "z", "y", "x" };

                  if (attrib->endianSwap == latte::SQ_ENDIAN_8IN16) {
                     decaf_check(channels == 2 || channels == 4);
                     val = val + "." + ChannelSel8In16[i];
                  } else if (attrib->endianSwap == latte::SQ_ENDIAN_8IN32) {
                     decaf_check(channels == 4);
                     val = val + "." + ChannelSel8In32[i];
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
               if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
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

            chanBitCount[i] = compBits;
         }
      }

      for (auto i = 0u; i < channels; ++i) {
         if (attrib->numFormat == latte::SQ_NUM_FORMAT_NORM) {
            uint32_t valMax = (1ul << chanBitCount[i]) - 1;

            if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
               chanVal[i] = fmt::format("clamp(float({}) / {}.0, -1.0, 1.0)", chanVal[i], valMax / 2);
            } else {
               chanVal[i] = fmt::format("float({}) / {}.0", chanVal[i], valMax);
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT_INT) {
            if (attrib->formatComp == latte::SQ_FORMAT_COMP_SIGNED) {
               chanVal[i] = "intBitsToFloat(int(" + chanVal[i] + "))";
            } else {
               chanVal[i] = "uintBitsToFloat(uint(" + chanVal[i] + "))";
            }
         } else if (attrib->numFormat == latte::SQ_NUM_FORMAT_SCALED) {
            chanVal[i] = "float(" + chanVal[i] + ")";
         } else {
            decaf_abort("Unexpected attribute number format");
         }
      }

      if (channels == 1) {
         out << "float _" << name << " = " << chanVal[0] << ";\n";
      } else if (channels == 2) {
         out << "vec2 _" << name << " = vec2(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ");\n";
      } else if (channels == 3) {
         out << "vec3 _" << name << " = vec3(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ",\n";
         out << "   " << chanVal[2] << ");\n";
      } else if (channels == 4) {
         out << "vec4 _" << name << " = vec4(\n";
         out << "   " << chanVal[0] << ",\n";
         out << "   " << chanVal[1] << ",\n";
         out << "   " << chanVal[2] << ",\n";
         out << "   " << chanVal[3] << ");\n";
      } else {
         decaf_abort("Unexpected format channel count");
      }
      name = "_" + name;

      // Write the register assignment
      out << "R[" << (i + 1) << "] = ";

      switch (channels) {
      case 1:
         out << "vec4(" << name << ", 0.0, 0.0, 1.0);\n";
         break;
      case 2:
         out << "vec4(" << name << ", 0.0, 1.0);\n";
         break;
      case 3:
         out << "vec4(" << name << ", 1.0);\n";
         break;
      case 4:
         out << name << ";\n";
         break;
      }
   }

   out << '\n' << shader.codeBody << '\n';

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_POS:
         if (!isScreenSpace) {
            out << "gl_Position = exp_position_" << exp.id << ";\n";
         } else {
            out << "gl_Position = (exp_position_" << exp.id << " - vec4(uViewport.xy, 0.0, 0.0)) * vec4(uViewport.zw, 1.0, 1.0);\n";
         }
         break;
      case latte::SQ_EXPORT_PARAM: {
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
            out << "vs_out_" << semanticId << " = exp_param_" << exp.id << ";\n";
         } else {
            // This just helps when debugging to understand why it is missing...
            out << "// vs_out_none = exp_param_" << exp.id << ";\n";
         }
      } break;
      case latte::SQ_EXPORT_PIXEL:
         decaf_abort("Unexpected pixel export in vertex shader.");
      }
   }

   out << "}\n";
   out << "/* VERTEX SHADER DISASSEMBLY\n" << vertex.disassembly << "\n*/\n";
   out << "/* FETCH SHADER DISASSEMBLY\n" << fetch.disassembly << "\n*/\n";
   vertex.code = out.str();
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
      auto resourceOffset = (latte::SQ_PS_TEX_RESOURCE_0 + i) * 7;
      auto sq_tex_resource_word0 = getRegister<latte::SQ_TEX_RESOURCE_WORD0_N>(latte::Register::SQ_TEX_RESOURCE_WORD0_0 + 4 * resourceOffset);

      shader.samplerDim[i] = sq_tex_resource_word0.DIM();
   }

   if (sq_config.DX9_CONSTS()) {
      shader.uniformRegistersEnabled = true;
   } else {
      shader.uniformBlocksEnabled = true;
   }

   pixel.disassembly = latte::disassemble(gsl::as_span(buffer, size));

   if (!glsl2::translate(shader, gsl::as_span(buffer, size))) {
      gLog->error("Failed to decode pixel shader\n{}", pixel.disassembly);
      return false;
   }

   pixel.samplerUsage = shader.samplerUsage;
   pixel.usedUniformBlocks = shader.usedUniformBlocks;

   fmt::MemoryWriter out;
   out << shader.fileHeader;
   out << "uniform float uAlphaRef;\n";

   auto z_order = db_shader_control.Z_ORDER();
   auto early_z = (z_order == latte::DB_EARLY_Z_THEN_LATE_Z || z_order == latte::DB_EARLY_Z_THEN_RE_Z);
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
            if (exp.type == latte::SQ_EXPORT_PIXEL && exp.id == 61) {
               gLog->debug("Ignoring early-Z because shader writes gl_FragDepth");
               early_z = false;
               break;
            }
         }
      }
      if (early_z) {
         out << "layout(early_fragment_tests) in;\n";
      }
   }

   if (spi_ps_in_control_0.POSITION_ENA()) {
      if (!spi_ps_in_control_0.POSITION_CENTROID()) {
         out << "layout(pixel_center_integer) ";
      }
      out << "in vec4 gl_FragCoord;\n";
   }

   // Pixel Shader Inputs
   std::array<bool, 256> semanticUsed = { false };
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
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

      out << "layout(location = " << vsOutputLoc << ")";

      if (spi_ps_input_cntl.FLAT_SHADE()) {
         out << " flat";
      }

      out << " in vec4 vs_out_" << semanticId << ";\n";
   }
   out << '\n';

   // Pixel Shader Exports
   auto maskBits = cb_shader_mask.value;

   for (auto i = 0; i < 8; ++i) {
      if (maskBits & 0xf) {
         out << "out vec4 ps_out_" << i << ";\n";
      }

      maskBits >>= 4;
   }
   out << '\n';

   out
      << "void main()\n"
      << "{\n"
      << shader.codeHeader;

   // Assign vertex shader output to our GPR
   for (auto i = 0u; i < spi_ps_in_control_0.NUM_INTERP(); ++i) {
      auto spi_ps_input_cntl = getRegister<latte::SPI_PS_INPUT_CNTL_0>(latte::Register::SPI_PS_INPUT_CNTL_0 + i * 4);
      uint8_t semanticId = spi_ps_input_cntl.SEMANTIC();
      decaf_check(semanticId != 0xff);

      auto vsOutputLoc = vertex.outputMap[semanticId];
      out << "R[" << i << "] = ";

      if (vsOutputLoc != 0xff) {
          out << "vs_out_" << semanticId;
      } else {
         if (spi_ps_input_cntl.DEFAULT_VAL() == 0) {
            out << "vec4(0, 0, 0, 0)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 1) {
            out << "vec4(0, 0, 0, 1)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 2) {
            out << "vec4(1, 1, 1, 0)";
         } else if (spi_ps_input_cntl.DEFAULT_VAL() == 3) {
            out << "vec4(1, 1, 1, 1)";
         } else {
            decaf_abort("Invalid PS input DEFAULT_VAL");
         }
      }

      out << ";\n";
   }

   if (spi_ps_in_control_0.POSITION_ENA()) {
      out << "R[" << spi_ps_in_control_0.POSITION_ADDR() << "] = gl_FragCoord;";
   }

   decaf_assert(!spi_ps_in_control_0.PARAM_GEN(),
                fmt::format("Unsupported spi_ps_in_control_0.PARAM_GEN {}, PARAM_GEN_ADDR {}",
                            spi_ps_in_control_0.PARAM_GEN(),
                            spi_ps_in_control_0.PARAM_GEN_ADDR()));
   decaf_check(!spi_ps_in_control_1.GEN_INDEX_PIX());
   decaf_check(!spi_ps_in_control_1.FIXED_PT_POSITION_ENA());

   out << '\n' << shader.codeBody << '\n';

   for (auto &exp : shader.exports) {
      switch (exp.type) {
      case latte::SQ_EXPORT_PIXEL:
         if (exp.id == 61) {
            if (!db_shader_control.Z_EXPORT_ENABLE()) {
               gLog->warn("Depth export is masked by db_shader_control");
            } else {
               out << "gl_FragDepth = exp_pixel_" << exp.id << ".x;\n";
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
                  out << "// Alpha Test ";

                  switch (sx_alpha_test_control.ALPHA_FUNC()) {
                  case latte::REF_NEVER:
                     out << "REF_NEVER\n";
                     out << "discard;\n";
                     break;
                  case latte::REF_LESS:
                     out << "REF_LESS\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w < uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_EQUAL:
                     out << "REF_EQUAL\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w == uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_LEQUAL:
                     out << "REF_LEQUAL\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w <= uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_GREATER:
                     out << "REF_GREATER\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w > uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_NOTEQUAL:
                     out << "REF_NOTEQUAL\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w != uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_GEQUAL:
                     out << "REF_GEQUAL\n";
                     out << "if (!(exp_pixel_" << exp.id << ".w >= uAlphaRef)) {\n";
                     out << "   discard;\n}\n";
                     break;
                  case latte::REF_ALWAYS:
                     out << "REF_ALWAYS\n";
                     break;
                  }
               }

               out
                  << "ps_out_" << exp.id << "." << strMask
                  << " = exp_pixel_" << exp.id << "." << strMask;

               out << ";\n";
            }
         }
         break;
      case latte::SQ_EXPORT_POS:
         decaf_abort("Unexpected position export in pixel shader.");
         break;
      case latte::SQ_EXPORT_PARAM:
         decaf_abort("Unexpected parameter export in pixel shader.");
         break;
      }
   }

   out << "}\n";

   out << "/* PIXEL SHADER DISASSEMBLY\n" << pixel.disassembly << "\n*/\n";

   pixel.code = out.str();
   return true;
}

} // namespace opengl

} // namespace gpu
