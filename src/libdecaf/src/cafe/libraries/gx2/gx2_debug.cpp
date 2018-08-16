#include "gx2.h"
#include "gx2_debug.h"
#include "gx2_enum_string.h"
#include "gx2_fetchshader.h"
#include "gx2_internal_cbpool.h"
#include "gx2_internal_gfd.h"
#include "gx2_shaders.h"
#include "gx2_texture.h"

#include "cafe/cafe_stackobject.h"
#include "cafe/libraries/coreinit/coreinit_snprintf.h"
#include "decaf_config.h"

#include <fstream>
#include <common/align.h>
#include <common/log.h>
#include <common/platform_dir.h>
#include <fmt/format.h>
#include <libcpu/mem.h>
#include <libgpu/latte/latte_disassembler.h>
#include <libgfd/gfd.h>

namespace cafe::gx2
{

void
GX2DebugTagUserString(uint32_t unk,
                      virt_ptr<const char> fmt,
                      var_args va)
{
   auto list = make_va_list(va);
   GX2DebugTagUserStringVA(unk, fmt, list);
   free_va_list(list);
}

void
GX2DebugTagUserStringVA(uint32_t unk,
                        virt_ptr<const char> fmt,
                        virt_ptr<va_list> vaList)
{
   StackArray<char, 0x404> buffer;;
   std::memset(buffer.getRawPointer(), 0, 0x404);

   if (fmt) {
      coreinit::internal::formatStringV(buffer, 0x3FF, fmt, vaList);
   }

   // Convert string to words!
   auto length = static_cast<uint32_t>(strlen(buffer.getRawPointer()));
   auto words = align_up(length + 1, 4) / 4;

   std::vector<uint32_t> strWords;
   strWords.resize(words, 0);
   std::memcpy(strWords.data(), buffer.getRawPointer(), length);

   // Write NOP packet
   internal::writePM4(latte::pm4::Nop {
      unk,
      gsl::make_span(strWords)
   });
}

namespace internal
{

static void
createDumpDirectory()
{
   platform::createDirectory("dump");
}

static std::string
pointerAsString(virt_ptr<const void> pointer)
{
   return fmt::format("{:08X}", virt_cast<virt_addr>(pointer));
}

static void
debugDumpData(const std::string &filename, virt_ptr<const void> data, size_t size)
{
   auto file = std::ofstream { filename, std::ofstream::out | std::ofstream::binary };
   file.write(static_cast<const char *>(data.getRawPointer()), size);
   file.close();
}

static void
debugDumpData(std::ofstream &file, virt_ptr<const void> data, size_t size)
{
   file.write(reinterpret_cast<const char *>(data.getRawPointer()), size);
}

void
debugDumpTexture(virt_ptr<const GX2Texture> texture)
{
   if (!decaf::config::gx2::dump_textures) {
      return;
   }

   createDumpDirectory();

   // Write text dump of GX2Texture structure to texture_X.txt
   auto filename = "texture_" + pointerAsString(texture);

   if (platform::fileExists("dump/" + filename + ".txt")) {
      return;
   }

   auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };
   auto format = fmt::MemoryWriter {};

   format
      << "surface.dim = " << gx2::to_string(texture->surface.dim) << '\n'
      << "surface.width = " << texture->surface.width << '\n'
      << "surface.height = " << texture->surface.height << '\n'
      << "surface.depth = " << texture->surface.depth << '\n'
      << "surface.mipLevels = " << texture->surface.mipLevels << '\n'
      << "surface.format = " << gx2::to_string(texture->surface.format) << '\n'
      << "surface.aa = " << gx2::to_string(texture->surface.aa) << '\n'
      << "surface.use = " << gx2::to_string(texture->surface.use) << '\n'
      << "surface.resourceFlags = " << texture->surface.resourceFlags << '\n'
      << "surface.imageSize = " << texture->surface.imageSize << '\n'
      << "surface.image = " << pointerAsString(texture->surface.image) << '\n'
      << "surface.mipmapSize = " << texture->surface.mipmapSize << '\n'
      << "surface.mipmaps = " << pointerAsString(texture->surface.mipmaps) << '\n'
      << "surface.tileMode = " << gx2::to_string(texture->surface.tileMode) << '\n'
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
   gfd::GFDFile gtx;
   gfd::GFDTexture gfdTexture;
   gx2ToGFDTexture(texture.getRawPointer(), gfdTexture);
   gtx.textures.push_back(gfdTexture);
   gfd::writeFile(gtx, "dump/" + filename + ".gtx");
}

static void
addShader(gfd::GFDFile &file,
          virt_ptr<const GX2VertexShader> shader)
{
   gfd::GFDVertexShader gfdShader;
   gx2ToGFDVertexShader(shader.getRawPointer(), gfdShader);
   file.vertexShaders.emplace_back(std::move(gfdShader));
}

static void
addShader(gfd::GFDFile &file,
          virt_ptr<const GX2PixelShader> shader)
{
   gfd::GFDPixelShader gfdShader;
   gx2ToGFDPixelShader(shader.getRawPointer(), gfdShader);
   file.pixelShaders.emplace_back(std::move(gfdShader));
}

static void
addShader(gfd::GFDFile &file,
          virt_ptr<const GX2GeometryShader> shader)
{
   gfd::GFDGeometryShader gfdShader;
   gx2ToGFDGeometryShader(shader.getRawPointer(), gfdShader);
   file.geometryShaders.emplace_back(std::move(gfdShader));
}

static void
addShader(gfd::GFDFile &file,
          virt_ptr<const GX2FetchShader> shader)
{
   // TODO: Add FetchShader support to .gsh
}

template<typename ShaderType>
static void
debugDumpShader(const std::string &filename,
                const std::string &info,
                virt_ptr<ShaderType> shader,
                bool isSubroutine = false)
{
   std::string output;

   // Write binary of shader data to shader_pixel_X.bin
   createDumpDirectory();

   if (platform::fileExists("dump/" + filename + ".bin")) {
      return;
   }

   gLog->debug("Dumping shader {}", filename);
   debugDumpData("dump/" + filename + ".bin", shader->data, shader->size);

   // Write GSH file
   gfd::GFDFile gsh;
   addShader(gsh, shader);
   gfd::writeFile(gsh, "dump/" + filename + ".gsh");

   // Write text of shader to shader_pixel_X.txt
   auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };

   // Disassemble
   output = latte::disassemble(gsl::make_span(shader->data.getRawPointer(), shader->size), isSubroutine);

   file
      << info << std::endl
      << "Disassembly:" << std::endl
      << output << std::endl;
}

static void
formatUniformBlocks(fmt::MemoryWriter &out,
                    uint32_t count,
                    virt_ptr<GX2UniformBlock> blocks)
{
   out << "  uniformBlockCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Block " << i << "\n"
         << "      name: " << blocks[i].name.getRawPointer() << "\n"
         << "      offset: " << blocks[i].offset << "\n"
         << "      size: " << blocks[i].size << "\n";
   }
}

static void
formatAttribVars(fmt::MemoryWriter &out,
                 uint32_t count,
                 virt_ptr<GX2AttribVar> vars)
{
   out << "  attribVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      name: " << vars[i].name.getRawPointer() << "\n"
         << "      type: " << gx2::to_string(vars[i].type) << "\n"
         << "      count: " << vars[i].count << "\n"
         << "      location: " << vars[i].location << "\n";
   }
}

static void
formatInitialValues(fmt::MemoryWriter &out,
                    uint32_t count,
                    virt_ptr<GX2UniformInitialValue> vars)
{
   out << "  intialValueCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      offset: " << vars[i].offset << "\n"
         << "      value: ("
            << vars[i].value[0] << ", "
            << vars[i].value[1] << ", "
            << vars[i].value[2] << ", "
            << vars[i].value[3] << ")"
            << "\n";
   }
}

static void
formatLoopVars(fmt::MemoryWriter &out,
               uint32_t count,
               virt_ptr<GX2LoopVar> vars)
{
   out << "  loopVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      value: " << vars[i].value << "\n"
         << "      offset: " << vars[i].offset << "\n";
   }
}

static void
formatUniformVars(fmt::MemoryWriter &out,
                  uint32_t count,
                  virt_ptr<GX2UniformVar> vars)
{
   out << "  uniformVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      name: " << vars[i].name.getRawPointer() << "\n"
         << "      type: " << gx2::to_string(vars[i].type) << "\n"
         << "      count: " << vars[i].count << "\n"
         << "      offset: " << vars[i].offset << "\n"
         << "      block: " << vars[i].block << "\n";
   }
}

static void
formatSamplerVars(fmt::MemoryWriter &out,
                  uint32_t count,
                  virt_ptr<GX2SamplerVar> vars)
{
   out << "  samplerVarCount: " << count << "\n";

   for (auto i = 0u; i < count; ++i) {
      out << "    Var " << i << "\n"
         << "      name: " << vars[i].name.getRawPointer() << "\n"
         << "      type: " << gx2::to_string(vars[i].type) << "\n"
         << "      location: " << vars[i].location << "\n";
   }
}

void
debugDumpShader(virt_ptr<const GX2FetchShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2FetchShader:\n"
      << "  address: " << pointerAsString(shader->data) << "\n"
      << "  size: " << shader->size << "\n";

   debugDumpShader("shader_fetch_" + pointerAsString(shader),
                   out.str(),
                   shader,
                   true);
}

void
debugDumpShader(virt_ptr<const GX2PixelShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2PixelShader:\n"
      << "  address: " << pointerAsString(shader->data) << "\n"
      << "  size: " << shader->size << "\n"
      << "  mode: " << gx2::to_string(shader->mode) << "\n";

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatInitialValues(out, shader->initialValueCount, shader->initialValues);
   formatLoopVars(out, shader->loopVarCount, shader->loopVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);

   debugDumpShader("shader_pixel_" + pointerAsString(shader),
                   out.str(),
                   shader);
}

void
debugDumpShader(virt_ptr<const GX2VertexShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::MemoryWriter out;
   out << "GX2VertexShader:\n"
      << "  address: " << pointerAsString(shader->data) << "\n"
      << "  size: " << shader->size << "\n"
      << "  mode: " << gx2::to_string(shader->mode) << "\n";

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatInitialValues(out, shader->initialValueCount, shader->initialValues);
   formatLoopVars(out, shader->loopVarCount, shader->loopVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);
   formatAttribVars(out, shader->attribVarCount, shader->attribVars);

   debugDumpShader("shader_vertex_" + pointerAsString(shader),
                   out.str(),
                   shader);
}

void
writeDebugMarker(std::string_view key,
                 uint32_t id)
{
   gLog->trace("CPU Debug Marker: {} {}", key, id);

   // PM4 commands must be 32 bit aligned, we need to copy it
   //  to a temporary local buffer so the gsl span doesn't
   //  overrun the variable which was passed by the user.
   static char tmpBuf[128];
   auto strLen = key.size() + 1;
   auto alignedStrLen = align_up(strLen, 4);
   memset(tmpBuf, 0, 128);
   memcpy(tmpBuf, key.data(), strLen);

   internal::writePM4(latte::pm4::DecafDebugMarker {
      id,
      gsl::make_span(tmpBuf, alignedStrLen),
   });
}

} // namespace internal

void
Library::registerDebugSymbols()
{
   RegisterFunctionExport(GX2DebugTagUserString);
   RegisterFunctionExport(GX2DebugTagUserStringVA);
}

} // namespace cafe::gx2
