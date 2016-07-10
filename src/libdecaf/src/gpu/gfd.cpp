#include "gfd.h"
#include "common/binaryfile.h"
#include "common/decaf_assert.h"
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_shaders.h"
#include <spdlog/details/format.h>

namespace gfd
{

const uint32_t
FileHeader::Magic;

const uint32_t
BlockHeader::Magic;

/*
Version 0:
GX2VertexShader->regs is 0xF0 bytes
GX2PixelShader is 0xD8 bytes (missing the UNK 4x4 at end)

Version 1:
GX2VertexShader->regs is 0xD0 bytes
GX2PixelShader is 0xE8 bytes
*/

bool
File::read(const std::string &filename)
{
   BinaryFile file;

   if (!file.open(filename)) {
      return false;
   }

   file.read(header);

   if (header.magic != FileHeader::Magic) {
      return false;
   }

   if (header.headerSize != sizeof(FileHeader)) {
      return false;
   }

   while (!file.eof()) {
      Block block;
      file.read(block.header);

      if (block.header.magic != BlockHeader::Magic) {
         return false;
      }

      if (block.header.headerSize != sizeof(BlockHeader)) {
         return false;
      }

      if (block.header.dataSize) {
         file.read(block.data, block.header.dataSize);
      }

      blocks.emplace_back(block);
   }

   // TODO: Repair pointers inside blocks
   return true;
}

bool
File::write(const std::string &filename)
{
   auto file = std::ofstream { filename, std::ofstream::out | std::ofstream::binary };

   if (!file.is_open()) {
      return false;
   }

   // Write file header
   file.write(reinterpret_cast<char*>(&header), sizeof(gfd::FileHeader));

   // Write blocks
   for (auto &block : blocks) {
      block.header.dataSize = block.data.size();
      file.write(reinterpret_cast<char*>(&block.header), sizeof(gfd::BlockHeader));
      file.write(reinterpret_cast<char*>(block.data.data()), block.data.size());
   }

   // Write EOF block
   auto eof = gfd::BlockHeader {};
   eof.type = gfd::BlockType::EndOfFile;
   file.write(reinterpret_cast<char*>(&eof), sizeof(gfd::BlockHeader));
   return true;
}

uint32_t File::getUniqueID()
{
   return indices.id++;
}

uint32_t File::getIndex(BlockType::Type type)
{
   switch (type) {
   case BlockType::FetchShaderHeader:
      return indices.fetch++;
   case BlockType::GeometryShaderHeader:
      return indices.geometry++;
   case BlockType::PixelShaderHeader:
      return indices.pixel++;
   case BlockType::VertexShaderHeader:
      return indices.vertex++;
   case BlockType::TextureHeader:
      return indices.texture++;
   default:
      decaf_abort(fmt::format("Invalid BlockType {}", static_cast<int>(type)));
   }
}

void
File::add(const gx2::GX2Texture *texture)
{
   // Add texture header block
   gfd::Block header;
   header.header.type = BlockType::TextureHeader;
   header.header.id = getUniqueID();
   header.header.index = getIndex(header.header.type);
   header.data.resize(sizeof(gx2::GX2Texture));
   std::memcpy(header.data.data(), texture, sizeof(gx2::GX2Texture));
   blocks.emplace_back(std::move(header));

   // Add texture image block
   if (texture->surface.image && texture->surface.imageSize) {
      gfd::Block image;
      image.header.type = gfd::BlockType::TextureImage;
      image.header.id = getUniqueID();
      image.header.index = header.header.index;
      image.data.resize(texture->surface.imageSize);
      std::memcpy(image.data.data(), texture->surface.image.get(), texture->surface.imageSize);
      blocks.emplace_back(std::move(image));
   }

   // Add texture mipmap block
   if (texture->surface.mipmaps && texture->surface.mipmapSize) {
      gfd::Block image;
      image.header.type = gfd::BlockType::TextureMipmap;
      image.header.id = getUniqueID();
      image.header.index = header.header.index;
      image.data.resize(texture->surface.mipmapSize);
      std::memcpy(image.data.data(), texture->surface.mipmaps.get(), texture->surface.mipmapSize);
      blocks.emplace_back(std::move(image));
   }
}

void
File::add(const gx2::GX2FetchShader *shader)
{
   // Add shader header block
   gfd::Block header;
   header.header.type = gfd::BlockType::FetchShaderHeader;
   header.header.id = getUniqueID();
   header.header.index = getIndex(header.header.type);
   header.data.resize(sizeof(gx2::GX2FetchShader));
   std::memcpy(header.data.data(), shader, sizeof(gx2::GX2FetchShader));
   blocks.emplace_back(std::move(header));

   // Add shader program block
   if (shader->data && shader->size) {
      gfd::Block program;
      program.header.type = gfd::BlockType::FetchShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = header.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
File::add(const gx2::GX2VertexShader *shader)
{
   // Add shader header block
   gfd::Block header;
   header.header.type = gfd::BlockType::VertexShaderHeader;
   header.header.id = getUniqueID();
   header.header.index = getIndex(header.header.type);
   header.data.resize(sizeof(gx2::GX2VertexShader));
   std::memcpy(header.data.data(), shader, sizeof(gx2::GX2VertexShader));
   blocks.emplace_back(std::move(header));

   // Add shader program block
   if (shader->data && shader->size) {
      gfd::Block program;
      program.header.type = gfd::BlockType::VertexShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = header.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
File::add(const gx2::GX2PixelShader *shader)
{
   // Add shader header block
   gfd::Block header;
   header.header.type = gfd::BlockType::PixelShaderHeader;
   header.header.id = getUniqueID();
   header.header.index = getIndex(header.header.type);
   header.data.resize(sizeof(gx2::GX2PixelShader));
   std::memcpy(header.data.data(), shader, sizeof(gx2::GX2PixelShader));
   blocks.emplace_back(std::move(header));

   // Add shader program block
   if (shader->data && shader->size) {
      gfd::Block program;
      program.header.type = gfd::BlockType::PixelShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = header.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
File::add(const gx2::GX2GeometryShader *shader)
{
   // TODO: Enable when we have defined GX2GeometryShader
#if 0
   // Add shader header block
   gfd::Block header;
   header.header.type = gfd::BlockType::GeometryShaderHeader;
   header.header.id = getUniqueID();
   header.header.index = getIndex(header.header.type);
   header.data.resize(sizeof(GX2GeometryShader));
   std::memcpy(header.data.data(), shader, sizeof(GX2GeometryShader));
   blocks.emplace_back(std::move(header));

   // Add shader program block
   if (shader->data && shader->size) {
      gfd::Block program;
      program.header.type = gfd::BlockType::PixelShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = header.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
#endif
}

} // namespace gsh
