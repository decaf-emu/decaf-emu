#include <fstream>
#include <spdlog/spdlog.h>
#include "config.h"
#include "gpu/gfd.h"
#include "gpu/microcode/latte_decoder.h"
#include "gpu/opengl/glsl_generator.h"
#include "gx2_enum_string.h"
#include "gx2_texture.h"
#include "gx2_shaders.h"
#include "cpu/mem.h"
#include "platform/platform_dir.h"
#include "common/log.h"

namespace gx2
{

static void
createDumpDirectory()
{
   platform::createDirectory("dump");
}

static std::string
GX2PointerAsString(const void *pointer)
{
   fmt::MemoryWriter format;
   format.write("{:08X}", mem::untranslate(pointer));
   return format.str();
}

static void
GX2DebugDumpData(const std::string &filename, const void *data, size_t size)
{
   auto file = std::ofstream { filename, std::ofstream::out | std::ofstream::binary };
   file.write(static_cast<const char *>(data), size);
   file.close();
}

static void
GX2DebugDumpData(std::ofstream &file, const void *data, size_t size)
{
   file.write(reinterpret_cast<const char *>(data), size);
}

void
GX2DebugDumpTexture(const GX2Texture *texture)
{
   if (!config::gx2::dump_textures) {
      return;
   }

   createDumpDirectory();

   // Write text dump of GX2Texture structure to texture_X.txt
   auto filename = "texture_" + GX2PointerAsString(texture);

   if (platform::fileExists("dump/" + filename + ".txt")) {
      return;
   }

   auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };
   auto format = fmt::MemoryWriter {};

   format
      << "surface.dim = " << GX2EnumAsString(texture->surface.dim) << '\n'
      << "surface.width = " << texture->surface.width << '\n'
      << "surface.height = " << texture->surface.height << '\n'
      << "surface.depth = " << texture->surface.depth << '\n'
      << "surface.mipLevels = " << texture->surface.mipLevels << '\n'
      << "surface.format = " << GX2EnumAsString(texture->surface.format) << '\n'
      << "surface.aa = " << GX2EnumAsString(texture->surface.aa) << '\n'
      << "surface.use = " << GX2EnumAsString(texture->surface.use) << '\n'
      << "surface.resourceFlags = " << texture->surface.resourceFlags << '\n'
      << "surface.imageSize = " << texture->surface.imageSize << '\n'
      << "surface.image = " << GX2PointerAsString(texture->surface.image) << '\n'
      << "surface.mipmapSize = " << texture->surface.mipmapSize << '\n'
      << "surface.mipmaps = " << GX2PointerAsString(texture->surface.mipmaps) << '\n'
      << "surface.tileMode = " << GX2EnumAsString(texture->surface.tileMode) << '\n'
      << "surface.swizzle = " << texture->surface.swizzle << '\n'
      << "surface.alignment = " << texture->surface.alignment << '\n'
      << "surface.pitch = " << texture->surface.pitch << '\n'
      << "viewFirstMip = " << texture->viewFirstMip << '\n'
      << "viewNumMips = " << texture->viewNumMips << '\n'
      << "viewFirstSlice = " << texture->viewFirstSlice << '\n'
      << "viewNumSlices = " << texture->viewNumSlices << '\n';

   file << format.str();

   if (!texture->surface.image || !texture->surface.imageSize) {
      return;
   }

   // Write GTX file
   gfd::File gtx;
   gtx.add(texture);
   gtx.write("dump/" + filename + ".gtx");
}

template<typename ShaderType>
static void
GX2DebugDumpShader(const std::string &filename, const std::string &info, ShaderType *shader, latte::Shader::Type shaderType)
{
   std::string output;

   // Write binary of shader data to shader_pixel_X.bin
   createDumpDirectory();

   if (platform::fileExists("dump/" + filename + ".bin")) {
      return;
   }

   gLog->debug("Dumping shader {}", filename);

   GX2DebugDumpData("dump/" + filename + ".bin", shader->data, shader->size);

   // Write GSH file
   gfd::File gsh;
   gsh.add(shader);
   gsh.write("dump/" + filename + ".gsh");

   // Write text of shader to shader_pixel_X.txt
   auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };

   // Disassemble
   latte::Shader decoded;
   decoded.type = shaderType;
   latte::decode(decoded, gsl::as_span(shader->data.get(), shader->size));
   latte::disassemble(decoded, output);

   file
      << info << std::endl
      << "Disassembly:" << std::endl
      << output << std::endl;

   output.clear();

   // Decompiled
   gpu::opengl::glsl::generateBody(decoded, output);

   file
      << "Decompiled:" << std::endl
      << output << std::endl;
}

static void
formatUniformBlocks(fmt::MemoryWriter &out, uint32_t count, GX2UniformBlock *blocks)
{
   out << "  uniformBlockCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Block " << i << "\n"
         << "      name: " << blocks[i].name.get() << "\n"
         << "      offset: " << blocks[i].offset << "\n"
         << "      size: " << blocks[i].size << "\n";
   }
}

static void
formatUniformVars(fmt::MemoryWriter &out, uint32_t count, GX2UniformVar *vars)
{
   out << "  uniformVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      name: " << vars[i].name.get() << "\n"
         << "      type: " << GX2EnumAsString(vars[i].type) << "\n"
         << "      count: " << vars[i].count << "\n"
         << "      offset: " << vars[i].offset << "\n"
         << "      block: " << vars[i].block << "\n";
   }
}

static void
formatSamplerVars(fmt::MemoryWriter &out, uint32_t count, GX2SamplerVar *vars)
{
   out << "  samplerVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      name: " << vars[i].name.get() << "\n"
         << "      type: " << GX2EnumAsString(vars[i].type) << "\n"
         << "      location: " << vars[i].location << "\n";
   }
}

void
GX2DebugDumpShader(GX2FetchShader *shader)
{
   if (!config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2FetchShader:\n"
      << "  size: " << shader->size << "\n";

   GX2DebugDumpShader("shader_fetch_" + GX2PointerAsString(shader),
                      out.str(),
                      shader,
                      latte::Shader::Fetch);
}

void
GX2DebugDumpShader(GX2PixelShader *shader)
{
   if (!config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2PixelShader:\n"
      << "  size: " << shader->size << "\n"
      << "  mode: " << GX2EnumAsString(shader->mode) << "\n";

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);

   out << "  initialValueCount: " << shader->initialValueCount << "\n";
   out << "  loopVarCount: " << shader->loopVarCount << "\n";

   GX2DebugDumpShader("shader_pixel_" + GX2PointerAsString(shader),
                      out.str(),
                      shader,
                      latte::Shader::Pixel);
}

void
GX2DebugDumpShader(GX2VertexShader *shader)
{
   if (!config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2VertexShader:\n"
      << "  size: " << shader->size << "\n"
      << "  mode: " << GX2EnumAsString(shader->mode) << "\n";

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);

   out << "  initialValueCount: " << shader->initialValueCount << "\n";
   out << "  loopVarCount: " << shader->loopVarCount << "\n";

   GX2DebugDumpShader("shader_vertex_" + GX2PointerAsString(shader),
                      out.str(),
                      shader,
                      latte::Shader::Vertex);
}

} // namespace gx2
