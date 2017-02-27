#pragma once
#include <cstdint>
#include <vector>
#include <common/be_val.h>

namespace gx2
{

struct GX2Texture;
struct GX2FetchShader;
struct GX2VertexShader;
struct GX2PixelShader;
struct GX2GeometryShader;

} // namespace gx2

namespace gfd
{

namespace BlockType
{
enum Type : uint32_t
{
   EndOfFile               = 1,
   VertexShaderHeader      = 3,
   VertexShaderProgram     = 5,
   PixelShaderHeader       = 6,
   PixelShaderProgram      = 7,
   GeometryShaderHeader    = 8,
   GeometryShaderProgram   = 9,
   TextureHeader           = 11,
   TextureImage            = 12,
   TextureMipmap           = 13,

   // Custom types:
   FetchShaderHeader       = 100,
   FetchShaderProgram      = 101,
};
}

#pragma pack(push, 1)

struct FileHeader
{
   static const uint32_t Magic = 0x47667832;
   be_val<uint32_t> magic = Magic;
   be_val<uint32_t> headerSize = sizeof(FileHeader);
   be_val<uint32_t> version1 = 7;
   be_val<uint32_t> version2 = 1;
   be_val<uint32_t> version3 = 2;
   be_val<uint32_t> align = 0;
   be_val<uint32_t> unk1 = 0;
   be_val<uint32_t> unk2 = 0;
};

struct BlockHeader
{
   static const uint32_t Magic = 0x424C4B7B;
   be_val<uint32_t> magic = Magic;
   be_val<uint32_t> headerSize = sizeof(BlockHeader);
   be_val<uint32_t> version1 = 1;
   be_val<uint32_t> version2 = 0;
   be_val<BlockType::Type> type = BlockType::EndOfFile;
   be_val<uint32_t> dataSize = 0;
   be_val<uint32_t> id = 0;
   be_val<uint32_t> index = 0;
};

struct RelocationHeader
{
   static const uint32_t Magic = 0x7D424C4B;
   be_val<uint32_t> magic = Magic;
   be_val<uint32_t> headerSize = sizeof(RelocationHeader);
   be_val<uint32_t> unk1;
   be_val<uint32_t> unk2;
   be_val<uint32_t> unk3;
   be_val<uint32_t> textSize;
   be_val<uint32_t> textOffset;
   be_val<uint32_t> unk4;
   be_val<uint32_t> patchCount;
   be_val<uint32_t> patchOffset;
};

#pragma pack(pop)

struct Writer
{
   struct Block
   {
      BlockHeader header;
      std::vector<uint8_t> data;
   };

   FileHeader header;
   std::vector<Block> blocks;

   struct
   {
      uint32_t id = 0;
      uint32_t fetch = 0;
      uint32_t geometry = 0;
      uint32_t pixel = 0;
      uint32_t texture = 0;
      uint32_t vertex = 0;
   } indices;

   uint32_t getUniqueID();
   uint32_t getIndex(BlockType::Type type);

   bool write(const std::string &filename);

   void add(const gx2::GX2Texture *texture);
   void add(const gx2::GX2FetchShader *shader);
   void add(const gx2::GX2VertexShader *shader);
   void add(const gx2::GX2PixelShader *shader);
   void add(const gx2::GX2GeometryShader *shader);
};

struct Reader
{
   struct Block
   {
      BlockHeader *header;
      uint8_t *data;
   };

   FileHeader *header;
   std::vector<Block> blocks;

   bool parse(void *data, uint32_t size);
   void relocateBlock(Block &block);
};

} // namespace gfd
