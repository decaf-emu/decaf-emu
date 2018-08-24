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
   auto out = fmt::memory_buffer {};

   fmt::format_to(out, "surface.dim = {}\n", gx2::to_string(texture->surface.dim));
   fmt::format_to(out, "surface.width = {}\n", texture->surface.width);
   fmt::format_to(out, "surface.height = {}\n", texture->surface.height);
   fmt::format_to(out, "surface.depth = {}\n", texture->surface.depth);
   fmt::format_to(out, "surface.mipLevels = {}\n", texture->surface.mipLevels);
   fmt::format_to(out, "surface.format = {}\n", gx2::to_string(texture->surface.format));
   fmt::format_to(out, "surface.aa = {}\n", gx2::to_string(texture->surface.aa));
   fmt::format_to(out, "surface.use = {}\n", gx2::to_string(texture->surface.use));
   fmt::format_to(out, "surface.resourceFlags = {}\n", texture->surface.resourceFlags);
   fmt::format_to(out, "surface.imageSize = {}\n", texture->surface.imageSize);
   fmt::format_to(out, "surface.image = {}\n", pointerAsString(texture->surface.image));
   fmt::format_to(out, "surface.mipmapSize = {}\n", texture->surface.mipmapSize);
   fmt::format_to(out, "surface.mipmaps = {}\n", pointerAsString(texture->surface.mipmaps));
   fmt::format_to(out, "surface.tileMode = {}\n", gx2::to_string(texture->surface.tileMode));
   fmt::format_to(out, "surface.swizzle = {}\n", texture->surface.swizzle);
   fmt::format_to(out, "surface.alignment = {}\n", texture->surface.alignment);
   fmt::format_to(out, "surface.pitch = {}\n", texture->surface.pitch);
   fmt::format_to(out, "viewFirstMip = {}\n", texture->viewFirstMip);
   fmt::format_to(out, "viewNumMips = {}\n", texture->viewNumMips);
   fmt::format_to(out, "viewFirstSlice = {}\n", texture->viewFirstSlice);
   fmt::format_to(out, "viewNumSlices = {}\n", texture->viewNumSlices);

   file << std::string_view { out.data(), out.size() };

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
debugDumpShader(const std::string_view &filename,
                const std::string_view &info,
                virt_ptr<ShaderType> shader,
                bool isSubroutine = false)
{
   std::string output;

   // Write binary of shader data to shader_pixel_X.bin
   createDumpDirectory();

   auto outputBin = fmt::format("dump/{}.bin", filename);
   if (platform::fileExists(outputBin)) {
      return;
   }

   gLog->debug("Dumping shader {}", filename);
   debugDumpData(outputBin, shader->data, shader->size);

   // Write GSH file
   gfd::GFDFile gsh;
   addShader(gsh, shader);
   gfd::writeFile(gsh, fmt::format("dump/{}.gsh", filename));

   // Write text of shader to shader_pixel_X.txt
   auto file = std::ofstream { fmt::format("dump/{}.txt", filename), std::ofstream::out };

   // Disassemble
   output = latte::disassemble(gsl::make_span(shader->data.getRawPointer(), shader->size), isSubroutine);

   file
      << info << std::endl
      << "Disassembly:" << std::endl
      << output << std::endl;
}

static void
formatUniformBlocks(fmt::memory_buffer &out,
                    uint32_t count,
                    virt_ptr<GX2UniformBlock> blocks)
{
   fmt::format_to(out, "  uniformBlockCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Block {}\n", i);
      fmt::format_to(out, "      name: {}\n", blocks[i].name);
      fmt::format_to(out, "      offset: {}\n", blocks[i].offset);
      fmt::format_to(out, "      size: {}\n", blocks[i].size);
   }
}

static void
formatAttribVars(fmt::memory_buffer &out,
                 uint32_t count,
                 virt_ptr<GX2AttribVar> vars)
{
   fmt::format_to(out, "  attribVarCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Var {}\n", i);
      fmt::format_to(out, "      name: {}\n", vars[i].name);
      fmt::format_to(out, "      type: {}\n", gx2::to_string(vars[i].type));
      fmt::format_to(out, "      count: {}\n", vars[i].count);
      fmt::format_to(out, "      location: {}\n", vars[i].location);
   }
}

static void
formatInitialValues(fmt::memory_buffer &out,
                    uint32_t count,
                    virt_ptr<GX2UniformInitialValue> vars)
{
   fmt::format_to(out, "  intialValueCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Var {}\n", i);
      fmt::format_to(out, "      offset: {}\n", vars[i].offset);
      fmt::format_to(out, "      value: ({}, {}, {}, {})\n",
                     vars[i].value[0], vars[i].value[1],
                     vars[i].value[2], vars[i].value[3]);
   }
}

static void
formatLoopVars(fmt::memory_buffer &out,
               uint32_t count,
               virt_ptr<GX2LoopVar> vars)
{
   fmt::format_to(out, "  loopVarCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Var {}\n", i);
      fmt::format_to(out, "      value: {}\n", vars[i].value);
      fmt::format_to(out, "      offset: {}\n", vars[i].offset);
   }
}

static void
formatUniformVars(fmt::memory_buffer &out,
                  uint32_t count,
                  virt_ptr<GX2UniformVar> vars)
{
   fmt::format_to(out, "  uniformVarCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Var {}\n", i);
      fmt::format_to(out, "      name: {}\n", vars[i].name);
      fmt::format_to(out, "      type: {}\n", gx2::to_string(vars[i].type));
      fmt::format_to(out, "      count: {}\n", vars[i].count);
      fmt::format_to(out, "      offset: {}\n", vars[i].offset);
      fmt::format_to(out, "      block: {}\n", vars[i].block);
   }
}

static void
formatSamplerVars(fmt::memory_buffer &out,
                  uint32_t count,
                  virt_ptr<GX2SamplerVar> vars)
{
   fmt::format_to(out, "  samplerVarCount: {}\n", count);

   for (auto i = 0u; i < count; ++i) {
      fmt::format_to(out, "    Var {}\n", i);
      fmt::format_to(out, "      name: {}\n", vars[i].name);
      fmt::format_to(out, "      type: {}\n", gx2::to_string(vars[i].type));
      fmt::format_to(out, "      location: {}\n", vars[i].location);
   }
}

void
debugDumpShader(virt_ptr<const GX2FetchShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::memory_buffer out;
   fmt::format_to(out, "GX2FetchShader:\n");
   fmt::format_to(out, "  address: {}\n", pointerAsString(shader->data));
   fmt::format_to(out, "  size: {}\n", shader->size);

   debugDumpShader("shader_fetch_" + pointerAsString(shader),
                   std::string_view { out.data(), out.size() },
                   shader,
                   true);
}

void
debugDumpShader(virt_ptr<const GX2PixelShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::memory_buffer out;
   fmt::format_to(out, "GX2PixelShader:\n");
   fmt::format_to(out, "  address: {}\n", pointerAsString(shader->data));
   fmt::format_to(out, "  size: {}\n", shader->size);
   fmt::format_to(out, "  mode: {}\n", gx2::to_string(shader->mode));

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatInitialValues(out, shader->initialValueCount, shader->initialValues);
   formatLoopVars(out, shader->loopVarCount, shader->loopVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);

   debugDumpShader("shader_pixel_" + pointerAsString(shader),
                   std::string_view { out.data(), out.size() },
                   shader);
}

void
debugDumpShader(virt_ptr<const GX2VertexShader> shader)
{
   if (!decaf::config::gx2::dump_shaders) {
      return;
   }

   fmt::memory_buffer out;
   fmt::format_to(out, "GX2VertexShader:\n");
   fmt::format_to(out, "  address: {}\n", pointerAsString(shader->data));
   fmt::format_to(out, "  size: {}\n", shader->size);
   fmt::format_to(out, "  mode: {}\n", gx2::to_string(shader->mode));

   formatUniformBlocks(out, shader->uniformBlockCount, shader->uniformBlocks);
   formatUniformVars(out, shader->uniformVarCount, shader->uniformVars);
   formatInitialValues(out, shader->initialValueCount, shader->initialValues);
   formatLoopVars(out, shader->loopVarCount, shader->loopVars);
   formatSamplerVars(out, shader->samplerVarCount, shader->samplerVars);
   formatAttribVars(out, shader->attribVarCount, shader->attribVars);

   debugDumpShader("shader_vertex_" + pointerAsString(shader),
                   std::string_view { out.data(), out.size() },
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
