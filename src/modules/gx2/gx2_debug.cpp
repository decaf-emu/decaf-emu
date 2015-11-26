#include <fstream>
#include <spdlog/spdlog.h>
#include <Windows.h>

#include "config.h"
#include "gx2_enum_string.h"
#include "gx2_texture.h"
#include "gx2_shaders.h"
#include "gpu/latte.h"
#include "gpu/opengl/glsl_generator.h"
#include "memory_translate.h"
#include "gpu/latte_format.h"
#include "gpu/gfd.h"

#pragma pack(1)

struct DdsPixelFormat
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwFourCC;
   uint32_t	dwRGBBitCount;
   uint32_t	dwRBitMask;
   uint32_t	dwGBitMask;
   uint32_t	dwBBitMask;
   uint32_t	dwABitMask;
};

struct DdsHeader
{
   uint32_t	dwSize;
   uint32_t	dwFlags;
   uint32_t	dwHeight;
   uint32_t	dwWidth;
   uint32_t	dwPitchOrLinearSize;
   uint32_t	dwDepth;
   uint32_t	dwMipMapCount;
   uint32_t	dwReserved1[11];
   DdsPixelFormat	ddspf;
   uint32_t	dwCaps;
   uint32_t	dwCaps2;
   uint32_t	dwCaps3;
   uint32_t	dwCaps4;
   uint32_t	dwReserved2;
};

static_assert(sizeof(DdsHeader) == 124, "dds header should be 124 bytes long");

#pragma pack()

static void
GX2CreateDumpDirectory()
{
   CreateDirectory(TEXT("dump"), NULL);
}

static std::string
GX2PointerAsString(const void *pointer)
{
   fmt::MemoryWriter format;
   format.write("{:08X}", memory_untranslate(pointer));
   return format.str();
}

static void
GX2DebugDumpData(const std::string &filename, const void *data, size_t size)
{
   auto file = std::ofstream { filename, std::ofstream::out | std::ofstream::binary };
   file.write(static_cast<const char *>(data), size);
}

static void
GX2DebugDumpData(std::ofstream &file, const void *data, size_t size)
{
   file.write(reinterpret_cast<const char *>(data), size);
}

static void
GX2DebugDumpGTX(const GX2Texture *texture)
{
   auto filename = "texture_" + GX2PointerAsString(texture);
   auto file = std::ofstream { "dump/" + filename + ".gtx", std::ofstream::out | std::ofstream::binary };

   // Write file header
   gfd::FileHeader header;
   memset(&header, 0, sizeof(gfd::FileHeader));
   header.magic = gfd::FileHeader::Magic;
   header.headerSize = sizeof(gfd::FileHeader);
   header.version1 = 7;
   header.version2 = 1;
   header.version3 = 2;
   file.write(reinterpret_cast<char*>(&header), sizeof(gfd::FileHeader));

   // Write TextureHeader
   gfd::BlockHeader block;
   memset(&block, 0, sizeof(gfd::FileHeader));
   block.magic = gfd::BlockHeader::Magic;
   block.headerSize = sizeof(gfd::BlockHeader);
   block.version1 = 1;
   block.version2 = 0;
   block.type = gfd::BlockType::TextureHeader;
   block.dataSize = sizeof(GX2Texture);
   file.write(reinterpret_cast<const char*>(&block), sizeof(gfd::BlockHeader));
   file.write(reinterpret_cast<const char*>(texture), sizeof(GX2Texture));

   // Write TextureImage
   memset(&block, 0, sizeof(gfd::FileHeader));
   block.magic = gfd::BlockHeader::Magic;
   block.headerSize = sizeof(gfd::BlockHeader);
   block.version1 = 1;
   block.version2 = 0;
   block.type = gfd::BlockType::TextureImage;
   block.dataSize = texture->surface.imageSize;
   file.write(reinterpret_cast<char*>(&block), sizeof(gfd::BlockHeader));
   file.write(reinterpret_cast<char*>(texture->surface.image.get()), texture->surface.imageSize);

   // Write EOF block
   memset(&block, 0, sizeof(gfd::FileHeader));
   block.magic = gfd::BlockHeader::Magic;
   block.headerSize = sizeof(gfd::BlockHeader);
   block.version1 = 1;
   block.version2 = 0;
   block.type = gfd::BlockType::EndOfFile;
   file.write(reinterpret_cast<char*>(&block), sizeof(gfd::BlockHeader));
}

void
GX2DebugDumpTexture(const GX2Texture *texture)
{
   if (!config::gx2::dump_textures) {
      return;
   }

   GX2CreateDumpDirectory();

   // Write text dump of GX2Texture structure to texture_X.txt
   auto filename = "texture_" + GX2PointerAsString(texture);
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

   // Write GTX
   GX2DebugDumpGTX(texture);

   // Write DDS
   /*std::vector<uint8_t> data;
   size_t rowPitch;
   latte::untileSurface(&texture->surface, data, rowPitch);

   DdsHeader ddsHeader;
   memset(&ddsHeader, 0, sizeof(ddsHeader));
   ddsHeader.dwSize = sizeof(ddsHeader);
   ddsHeader.dwFlags = 0x1 | 0x2 | 0x4 | 0x1000 | 0x80000;
   ddsHeader.dwHeight = texture->surface.height;
   ddsHeader.dwWidth = texture->surface.width;
   ddsHeader.dwPitchOrLinearSize = (uint32_t)data.size();
   ddsHeader.ddspf.dwSize = sizeof(ddsHeader.ddspf);
   ddsHeader.ddspf.dwFlags = 0x1 | 0x4;
   ddsHeader.dwCaps = 0x1000;

   switch (texture->surface.format) {
   case GX2SurfaceFormat::UNORM_BC1: ddsHeader.ddspf.dwFourCC = '1TXD'; break;
   case GX2SurfaceFormat::UNORM_BC2:ddsHeader.ddspf.dwFourCC = '3TXD'; break;
   case GX2SurfaceFormat::UNORM_BC3:ddsHeader.ddspf.dwFourCC = '5TXD'; break;
   case GX2SurfaceFormat::UNORM_BC4:ddsHeader.ddspf.dwFourCC = '1ITA'; break;
   case GX2SurfaceFormat::UNORM_BC5:ddsHeader.ddspf.dwFourCC = '2ITA'; break;
   default:
      return;
   }

   auto binaryDds = std::ofstream{ "dump/" + filename + ".dds", std::ofstream::out | std::ofstream::binary };
   GX2DebugDumpData(binaryDds, "DDS ", 4);
   GX2DebugDumpData(binaryDds, &ddsHeader, sizeof(ddsHeader));
   GX2DebugDumpData(binaryDds, &data[0], data.size());*/
}

static void
GX2DebugDumpShader(const std::string &filename, const std::string &info, uint8_t *data, size_t size)
{
   std::string output;

   // Write binary of shader data to shader_pixel_X.bin
   GX2CreateDumpDirectory();
   GX2DebugDumpData("dump/" + filename + ".bin", data, size);

   // Write text of shader to shader_pixel_X.txt
   auto file = std::ofstream { "dump/" + filename + ".txt", std::ofstream::out };

   // Disassemble
   latte::disassemble(output, { data, size });

   file
      << info << std::endl
      << "Disassembly:" << std::endl
      << output << std::endl;

   output.clear();

   // Decompiled
   auto decompiled = latte::Shader { };
   latte::decode(decompiled, latte::Shader::Vertex, { data, size });
   gpu::opengl::glsl::generateBody(decompiled, output);

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

   GX2DebugDumpShader("shader_fetch_" + GX2PointerAsString(shader), out.str(), reinterpret_cast<uint8_t *>(shader->data.get()), shader->size);
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
   GX2DebugDumpShader("shader_pixel_" + GX2PointerAsString(shader), out.str(), shader->data, shader->size);
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

   GX2DebugDumpShader("shader_vertex_" + GX2PointerAsString(shader), out.str(), shader->data, shader->size);
}
