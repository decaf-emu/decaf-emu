#if 0
#include "gfd.h"
#include <common/decaf_assert.h>
#include "modules/gx2/gx2_texture.h"
#include "modules/gx2/gx2_shaders.h"
#include <fstream>
#include <spdlog/fmt/fmt.h>

namespace gfd
{

const uint32_t
FileHeader::Magic;

const uint32_t
BlockHeader::Magic;

const uint32_t
RelocationHeader::Magic;

/*
For now, let's only support Version1 because it matches our GX2 structs.

BlockHeader::Version1 = 0:
GX2VertexShader->regs is 0xF0 bytes
GX2PixelShader is 0xD8 bytes (missing the UNK 4x4 at end)

BlockHeader::Version1 = 1:
GX2VertexShader->regs is 0xD0 bytes
GX2PixelShader is 0xE8 bytes
*/

bool
Reader::parse(void *data, uint32_t size)
{
   auto data8 = reinterpret_cast<uint8_t *>(data);

   if (size < sizeof(FileHeader)) {
      return false;
   }

   header = reinterpret_cast<FileHeader *>(data8);

   if (header->magic != FileHeader::Magic) {
      return false;
   }

   if (header->headerSize != sizeof(FileHeader)) {
      return false;
   }

   decaf_check(header->version1 == 7);

   for (auto pos = sizeof(FileHeader); pos + sizeof(BlockHeader) < size; ) {
      auto blockHeader = reinterpret_cast<BlockHeader *>(data8 + pos);

      if (blockHeader->magic != BlockHeader::Magic) {
         return false;
      }

      if (blockHeader->headerSize != sizeof(BlockHeader)) {
         return false;
      }

      auto blockData = data8 + pos + sizeof(BlockHeader);

      if (blockHeader->dataSize == 0) {
         blockData = nullptr;
      }

      decaf_check(blockHeader->version1 == 1);

      blocks.push_back({ blockHeader, blockData });

      if (blockHeader->type == BlockType::VertexShaderHeader
       || blockHeader->type == BlockType::PixelShaderHeader
       || blockHeader->type == BlockType::GeometryShaderHeader
       || blockHeader->type == BlockType::TextureHeader
       || blockHeader->type == BlockType::FetchShaderHeader) {
         relocateBlock(blocks.back());
      }

      pos += blockHeader->headerSize + blockHeader->dataSize;
   }

   return true;
}

void
Reader::relocateBlock(Block &block)
{
   if (block.header->dataSize < sizeof(RelocationHeader)) {
      return;
   }

   auto headerOffset = block.header->dataSize - sizeof(RelocationHeader);
   auto relocationHeader = reinterpret_cast<RelocationHeader *>(block.data + headerOffset);

   if (relocationHeader->magic != RelocationHeader::Magic) {
      return;
   }

   auto patchOffset = relocationHeader->patchOffset & 0x000FFFFF;
   auto patches = reinterpret_cast<be_val<uint32_t> *>(block.data + patchOffset);

   for (auto i = 0u; i < relocationHeader->patchCount; ++i) {
      auto location = patches[i] & 0x000FFFFF;

      if (!location) {
         continue;
      }

      auto data = reinterpret_cast<be_val<uint32_t> *>(block.data + location);
      auto value = *data & 0x000FFFFF;

      assert(value < block.header->dataSize);
      *data = mem::untranslate(block.data + value);
   }
}

bool
Writer::write(const std::string &filename)
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
      file.write(reinterpret_cast<char*>(&block.header), sizeof(BlockHeader));
      file.write(reinterpret_cast<char*>(block.data.data()), block.data.size());
   }

   // Write EOF block
   auto eof = BlockHeader {};
   eof.type = BlockType::EndOfFile;
   file.write(reinterpret_cast<char*>(&eof), sizeof(BlockHeader));
   return true;
}

uint32_t Writer::getUniqueID()
{
   return indices.id++;
}

uint32_t Writer::getIndex(BlockType::Type type)
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
Writer::add(const gx2::GX2Texture *texture)
{
   // Add texture header block
   Block headerBlock;
   headerBlock.header.type = BlockType::TextureHeader;
   headerBlock.header.id = getUniqueID();
   headerBlock.header.index = getIndex(headerBlock.header.type);
   headerBlock.data.resize(sizeof(gx2::GX2Texture));
   std::memcpy(headerBlock.data.data(), texture, sizeof(gx2::GX2Texture));
   blocks.emplace_back(std::move(headerBlock));

   // Add texture image block
   if (texture->surface.image && texture->surface.imageSize) {
      Block image;
      image.header.type = BlockType::TextureImage;
      image.header.id = getUniqueID();
      image.header.index = headerBlock.header.index;
      image.data.resize(texture->surface.imageSize);
      std::memcpy(image.data.data(), texture->surface.image.get(), texture->surface.imageSize);
      blocks.emplace_back(std::move(image));
   }

   // Add texture mipmap block
   if (texture->surface.mipmaps && texture->surface.mipmapSize) {
      Block image;
      image.header.type = BlockType::TextureMipmap;
      image.header.id = getUniqueID();
      image.header.index = headerBlock.header.index;
      image.data.resize(texture->surface.mipmapSize);
      std::memcpy(image.data.data(), texture->surface.mipmaps.get(), texture->surface.mipmapSize);
      blocks.emplace_back(std::move(image));
   }
}

void
Writer::add(const gx2::GX2FetchShader *shader)
{
   // Add shader header block
   Block headerBlock;
   headerBlock.header.type = BlockType::FetchShaderHeader;
   headerBlock.header.id = getUniqueID();
   headerBlock.header.index = getIndex(headerBlock.header.type);
   headerBlock.data.resize(sizeof(gx2::GX2FetchShader));
   std::memcpy(headerBlock.data.data(), shader, sizeof(gx2::GX2FetchShader));
   blocks.emplace_back(std::move(headerBlock));

   // Add shader program block
   if (shader->data && shader->size) {
      Block program;
      program.header.type = BlockType::FetchShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = headerBlock.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
Writer::add(const gx2::GX2VertexShader *shader)
{
   // Add shader header block
   Block headerBlock;
   headerBlock.header.type = BlockType::VertexShaderHeader;
   headerBlock.header.id = getUniqueID();
   headerBlock.header.index = getIndex(headerBlock.header.type);
   headerBlock.data.resize(sizeof(gx2::GX2VertexShader));
   std::memcpy(headerBlock.data.data(), shader, sizeof(gx2::GX2VertexShader));
   blocks.emplace_back(std::move(headerBlock));

   // Add shader program block
   if (shader->data && shader->size) {
      Block program;
      program.header.type = BlockType::VertexShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = headerBlock.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
Writer::add(const gx2::GX2PixelShader *shader)
{
   // Add shader header block
   Block headerBlock;
   headerBlock.header.type = BlockType::PixelShaderHeader;
   headerBlock.header.id = getUniqueID();
   headerBlock.header.index = getIndex(headerBlock.header.type);
   headerBlock.data.resize(sizeof(gx2::GX2PixelShader));
   std::memcpy(headerBlock.data.data(), shader, sizeof(gx2::GX2PixelShader));
   blocks.emplace_back(std::move(headerBlock));

   // Add shader program block
   if (shader->data && shader->size) {
      Block program;
      program.header.type = BlockType::PixelShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = headerBlock.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

void
Writer::add(const gx2::GX2GeometryShader *shader)
{
   // Add shader header block
   Block headerBlock;
   headerBlock.header.type = BlockType::GeometryShaderHeader;
   headerBlock.header.id = getUniqueID();
   headerBlock.header.index = getIndex(headerBlock.header.type);
   headerBlock.data.resize(sizeof(gx2::GX2GeometryShader));
   std::memcpy(headerBlock.data.data(), shader, sizeof(gx2::GX2GeometryShader));
   blocks.emplace_back(std::move(headerBlock));

   // Add shader program block
   if (shader->data && shader->size) {
      Block program;
      program.header.type = BlockType::GeometryShaderProgram;
      program.header.id = getUniqueID();
      program.header.index = headerBlock.header.index;
      program.data.resize(shader->size);
      std::memcpy(program.data.data(), shader->data.get(), shader->size);
      blocks.emplace_back(std::move(program));
   }
}

} // namespace gsh
#endif
